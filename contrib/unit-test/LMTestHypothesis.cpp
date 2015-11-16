/****************************************************
 * Moses - factored phrase-based language decoder   *
 * Copyright (C) 2015 University of Edinburgh       *
 * Licensed under GNU LGPL Version 2.1, see COPYING *
 ****************************************************/

/**
 * \brief Simple unit tests for language models, especially KenLM.
 *
 * Copied from LMTest.cpp, TODO: document this file.
 *
 * This file demonstrates how to create and use a LanguageModel,
 * and verifies whether resulting scores are correct.
 *
 * Currently only tests KenLM. Adapt it if you want to test your own LM.
 *
 */

// must be defined before including boost/test/unit_test.hpp
#define BOOST_TEST_MODULE LMTestHypothesis
#include <boost/test/unit_test.hpp>

#include <iostream>
#include <fstream>
#include <cassert>
#include <cmath>
#include <vector>
#include <utility>

#include "moses/LM/Ken.h"
#include "moses/Bitmaps.h"
#include "lm/model.hh"

#include "LMTestArpa.h"
#include "LMTest.h"

#include "MockManager.h"

// UGLY remove after cleanup of Word
#include "moses/StaticData.h"


// relative tolerance for float comparisons
#define TOLERANCE 1e-3


namespace utf = boost::unit_test;
namespace tt = boost::test_tools;

using namespace Moses;

/*
typedef std::pair<size_t, size_t> Align;
typedef std::pair<std::string, Align> PhraseAlign;
*/

struct PhraseAlign {
  std::string targetWords;
  Range sourceRange;

  PhraseAlign(std::string _targetWords, Range _sourceRange): targetWords(_targetWords), sourceRange(_sourceRange) {}
};

BOOST_AUTO_TEST_SUITE(LMTestHypothesisSuite)

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

  void testHypothesis(const Sentence& source, const std::vector<PhraseAlign>& phrases) {
    Hypothesis *hypo = NULL;
    size_t nhypo = 0;

    // vector<bool> source.m_sourceCompleted
    BOOST_CHECK(source.m_sourceCompleted.empty());
    Bitmaps bitmaps(source.GetSize(), source.m_sourceCompleted);
    Manager &noMan = *static_cast<Manager *>(NULL);
    TranslationOption initialTransOpt;

    const Bitmap &initialBitmap = bitmaps.GetInitialBitmap();
    hypo = new Hypothesis(noMan, source, initialTransOpt, initialBitmap, /* id = */ ++nhypo);

    for(auto pa: phrases) {
      Hypothesis* prevHypo = hypo;
      // add sourceRange to coverage of previous Hypothesis
      const Bitmap &bitmap = bitmaps.GetBitmap(prevHypo->GetWordsBitmap(), pa.sourceRange);

      TargetPhrase targetPhrase;
      targetPhrase.CreateFromString(Input, m_factorOrder, pa.targetWords, NULL);

      TranslationOption translationOption(pa.sourceRange, targetPhrase);

      hypo = new Hypothesis(*prevHypo, translationOption, bitmap, /* id = */ ++nhypo);
    }
  }

  ~KenLMFixture() {
    delete m_lm;
  }

protected:
  LanguageModel *m_lm; ///< Used by every BOOST_FIXTURE_TEST_CASE below.
  std::vector<FactorType> m_factorOrder; ///< indices of factors in each token, delimited by | (e.g. "surface|POS")
};

BOOST_FIXTURE_TEST_CASE(TestHypothesisChain, KenLMFixture) {
  std::vector<PhraseAlign> phrases;

  Sentence sentence;
  sentence.CreateFromString(m_factorOrder, "sjohn sgave smary sa sbook s.");

  phrases.push_back(PhraseAlign("john", Range(0, 0)));
  phrases.push_back(PhraseAlign("gave", Range(1, 1)));
  phrases.push_back(PhraseAlign("mary", Range(2, 2)));
  phrases.push_back(PhraseAlign("a book", Range(3, 4)));
  phrases.push_back(PhraseAlign(".", Range(5, 5)));

  testHypothesis(sentence, phrases);

  std::cout << "created Hypothesis chain" << std::endl;
}

BOOST_FIXTURE_TEST_CASE(TestHypothesis, KenLMFixture) {
  std::vector<std::string> phraseStrings;
  std::vector<Phrase> phrases;

  Phrase phrase; // XX
  float fullScore, ngramScore;
  size_t oovCount;

  // dream up some target phrase sequence
  phraseStrings.push_back("john");
  phraseStrings.push_back("gave");
  phraseStrings.push_back("mary");
  phraseStrings.push_back("a book");
  phraseStrings.push_back(".");

  // create the Phrase objects
  for(auto s: phraseStrings) {
    phrases.push_back(Phrase());
    phrases.back().CreateFromString(Output, m_factorOrder, s, /* lhs = */ NULL);
  }

  std::cout << "created Phrase objects" << std::endl;

  Sentence sentence; //(0, "john gave mary a book .", opts, &m_factorOrder);
  sentence.CreateFromString(m_factorOrder, "john gave mary a book .");

  std::cout << "created Sentence object from string" << std::endl;

  //const InputType &sentence = *static_cast<const InputType *>(NULL);
  TranslationOption initialTransOpt;
  Bitmap initialBitmap(sentence.GetSize());
  Hypothesis *empty = new Hypothesis(*static_cast<Manager *>(NULL), sentence, initialTransOpt, initialBitmap, /* id = */ 1);

  std::cout << "created Hypothesis" << std::endl;

  /////////////////////////////////////////
  /////////////////////////////////////////

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
