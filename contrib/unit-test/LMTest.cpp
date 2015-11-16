/****************************************************
 * Moses - factored phrase-based language decoder   *
 * Copyright (C) 2015 University of Edinburgh       *
 * Licensed under GNU LGPL Version 2.1, see COPYING *
 ****************************************************/

/**
 * \brief Simple unit tests for language models, especially KenLM.
 *
 * This file demonstrates how to create and use a LanguageModel,
 * and verifies whether resulting scores are correct.
 *
 * Currently only tests KenLM. Adapt it if you want to test your own LM.
 *
 */

// must be defined before including boost/test/unit_test.hpp
#define BOOST_TEST_MODULE LMTest
#include <boost/test/unit_test.hpp>

#include <iostream>
#include <fstream>
#include <cassert>
#include <cmath>

#include "moses/LM/Ken.h"
#include "lm/model.hh"

#include "LMTestArpa.h"
#include "LMTest.h"

// UGLY remove after cleanup of Word
#include "moses/StaticData.h"


// relative tolerance for float comparisons
#define TOLERANCE 1e-3


namespace utf = boost::unit_test;
namespace tt = boost::test_tools;

using namespace Moses;


BOOST_AUTO_TEST_SUITE(LMTestSuite)

/**
 * \brief Sets up LanguageModelKen for each test case.
 *
 * In addition to the explicitly necessary setup visible in the code below,
 * LM usage currently also depends on these less-than-pretty static parts of Moses:
 *
 * * ScoreComponentCollection
 *     registration of LM feature function to obtain FFIndex (index into SCC: index, numScoreComponents)
 * * FactorCollection
 *     creation of globally-unique word IDs at LM loading time
 * * StaticData
 *     ScoreComponentCollection::GetWeightedScore() would use feature weights from StaticData -- for linking
 *     Word::CreateFromString() uses factorDelimiter ("|") from StaticData
 *
 * * Incremental.cpp (KenLM only)
 *     call to Manager::LMCallback() from Ken.cpp for Ken's incremental decoder implementation... :(
 *
 */
class KenLMFixture {
public:
  KenLMFixture() {
    std::string line = "KENLM name=LM0"; // factor=0, path=..., order=5
    std::string file = setupArpa();
    FactorType factorType = 0; // type (index) of surface factor
    bool lazy = false;

    m_factorOrder.push_back(factorType); // surface factor first, by convention

    // UGLY: remove this assert after cleaning up Word to avoid using StaticData.
    // make sure we linked against the MockStaticData.cpp implementation.
    assert(StaticData::Instance().GetFactorDelimiter() == "|");

    /*
     * The LanguageModel constructor registers itself with ScoreComponentCollection, which statically
     * holds the information about the index and numScoreComponents of each FeatureFunction in SCC.
     */
    m_lm = new LanguageModelKen<lm::ngram::ProbingModel>(line, file, factorType, lazy);
  }

  ~KenLMFixture() {
    delete m_lm;
  }

protected:
  LanguageModel *m_lm; ///< Used by every BOOST_FIXTURE_TEST_CASE below.
  std::vector<FactorType> m_factorOrder; ///< indices of factors in each token, delimited by | (e.g. "surface|POS")
};

/**
 * Test CalcScore() interface of LM with a full sentence.
 * Exercises n-gram probabilities, backoffs, and fallback to unigram.
 */
BOOST_FIXTURE_TEST_CASE(TestCalcScoreSentence, KenLMFixture) {
  Phrase phrase;
  float fullScore, ngramScore;
  size_t oovCount;

  std::string query = "<s> john gave mary a book . </s>";

  // split a string into tokens (in Phrase), and tokens into factors (in Word)
  phrase.CreateFromString(Input, m_factorOrder, query, /* lhs = */ NULL); // lhs is NT label for syntax rules only

  // CalcScore() computes Phrase score without an implicit <s> at the beginning
  m_lm->CalcScore(phrase, fullScore, ngramScore, oovCount);

  BOOST_CHECK(oovCount == 1); // "gave" is OOV in the query

  /*
   * at each fallback to unigram, backoff of previous word is applied.
   *
   * john=13 2 -1.255
   * gave=0  1 -1.832
   * mary=6  1 -1.531  // log p_b(mary) = -0.2
   * a=8     1 -1.455  // -0.2 backoff applied here
   * book=3  2 -1.243
   * .=5     2 -1.243
   * </s>=12 2 -0.954
   * Total:    -9.513 OOV: 1  (log_10 scores)
   */
  const float expectedScore = -9.513f * logf(10.0f);
  BOOST_CHECK_CLOSE(fullScore, expectedScore, TOLERANCE);

  // for bigram LM, fullScore == ngramScore, since log P(<s>)=0
  BOOST_CHECK_CLOSE(ngramScore, expectedScore, TOLERANCE);
}

/**
 * Test CalcScore() up to "mary", to see that -0.2 backoff is only applied at "a".
 */
BOOST_FIXTURE_TEST_CASE(TestCalcScoreBackoff, KenLMFixture) {
  Phrase phrase;
  float fullScore, ngramScore;
  size_t oovCount;

  std::string query = "<s> john gave mary";

  // split a string into tokens (in Phrase), and tokens into factors (in Word)
  phrase.CreateFromString(Input, m_factorOrder, query, /* lhs = */ NULL); // lhs is NT label for syntax rules only

  // CalcScore() computes Phrase score without an implicit <s> at the beginning
  m_lm->CalcScore(phrase, fullScore, ngramScore, oovCount);

  BOOST_CHECK(oovCount == 1); // "gave" is OOV in the query

  /*
   * at each fallback to unigram, backoff of previous word is applied.
   *
   * john=13 2 -1.255
   * gave=0  1 -1.832
   * mary=6  1 -1.531  // log p_b(mary) = -0.2, but not applied yet.
   * Total:    -4.618 OOV: 1  (log_10 scores)
   */
  const float expectedScore = -4.618f * logf(10.0f);
  BOOST_CHECK_CLOSE(fullScore, expectedScore, TOLERANCE);
  BOOST_CHECK_CLOSE(ngramScore, expectedScore, TOLERANCE);

  // TODO: with a trigram LM, we could test the subtle difference between ...
  // * sentence-start low-order n-grams,  -- no backoff applied
  // * low-order n-grams due to backoff
}

/**
 * Test ngramScore returned by CalcScore()
 */
BOOST_FIXTURE_TEST_CASE(TestCalcScoreNgram, KenLMFixture) {
  Phrase phrase;
  float fullScore, ngramScore;
  size_t oovCount;

  std::string query = "a book";

  // split a string into tokens (in Phrase), and tokens into factors (in Word)
  phrase.CreateFromString(Input, m_factorOrder, query, /* lhs = */ NULL); // lhs is NT label for syntax rules only

  // CalcScore() computes Phrase score without an implicit <s> at the beginning
  m_lm->CalcScore(phrase, fullScore, ngramScore, oovCount);

  /*
   * a=8     1 -1.255
   * book=3  2 -1.243
   * Total:    -2.498 OOV: 0  (log_10 scores)
   *
   * ngramScore -1.243 (excludes the first order-1 tokens)
   */
  BOOST_CHECK_CLOSE(fullScore, -2.498f * logf(10.0f), TOLERANCE);
  BOOST_CHECK_CLOSE(ngramScore, -1.243f * logf(10.0f), TOLERANCE);

  BOOST_CHECK(oovCount == 0);
}

BOOST_AUTO_TEST_SUITE_END()
