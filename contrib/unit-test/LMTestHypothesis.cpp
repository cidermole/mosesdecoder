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
 * * StatefulFeatureFunction
 *     registration of LM feature function to obtain EmptyHypothesisState() in Hypothesis constructor
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

  /**
   * Construct a chain of Hypotheses like done in phrase-based stack decoding (see SearchNormal).
   *
   * \param source        source words
   * \param targetPhrases sequence of target phrases and their alignment to source word ranges
   *
   * \return Entire chain of Hypotheses from the empty Hypothesis to the last phrase.
   */
  std::vector<Hypothesis *> hypoChain(const Sentence& source, const std::vector<PhraseAlign>& targetPhrases) {
    std::vector<Hypothesis *> hypotheses;
    Hypothesis *hypo = NULL;
    size_t nhypo = 0;

    // vector<bool> source.m_sourceCompleted
    // source words already covered before we even got started. Why would anybody want to specify these?
    BOOST_CHECK(source.m_sourceCompleted.empty());
    Bitmaps bitmaps(source.GetSize(), source.m_sourceCompleted);
    Manager &noMan = *static_cast<Manager *>(NULL);
    TranslationOption initialTransOpt;

    const Bitmap &initialBitmap = bitmaps.GetInitialBitmap();

    // this uses static registry of FFs from StatefulFeatureFunction::GetStatefulFeatureFunctions()
    // to obtain EmptyHypothesisState() of the LM.
    hypo = new Hypothesis(noMan, source, initialTransOpt, initialBitmap, /* id = */ ++nhypo);
    hypotheses.push_back(hypo);

    for(auto pa: targetPhrases) {
      Hypothesis* prevHypo = hypo;
      // add sourceRange to coverage of previous Hypothesis
      const Bitmap &bitmap = bitmaps.GetBitmap(prevHypo->GetWordsBitmap(), pa.sourceRange);

      TargetPhrase targetPhrase;
      targetPhrase.CreateFromString(Input, m_factorOrder, pa.targetWords, NULL);

      TranslationOption *translationOption = new TranslationOption(pa.sourceRange, targetPhrase);

      // these have NULL as FFState until we actually compute state
      hypo = new Hypothesis(*prevHypo, *translationOption, bitmap, /* id = */ ++nhypo);
      hypotheses.push_back(hypo);
    }

    return hypotheses;
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

  Sentence source;
  source.CreateFromString(m_factorOrder, "sjohn sgave smary sa sbook s.");

  // target phrases, with source word range
  phrases.push_back(PhraseAlign("john", Range(0, 0)));
  phrases.push_back(PhraseAlign("gave", Range(1, 1)));
  phrases.push_back(PhraseAlign("mary", Range(2, 2)));
  phrases.push_back(PhraseAlign("a book", Range(3, 4)));
  phrases.push_back(PhraseAlign(".", Range(5, 5)));

  std::vector<Hypothesis *> hypotheses = hypoChain(source, phrases);

  std::cout << "created Hypothesis chain" << std::endl;

  /*
   * Evaluate feature function on hypotheses.
   */
  const StatefulFeatureFunction &ff = *m_lm;
  // SCC registration done in LanguageModel constructor. Only once - static!
  //ScoreComponentCollection::RegisterScoreProducer(&ff);

  ScoreComponentCollection scoreBreakdown;

  // ffIndex is usually from the order of registration in StatefulFeatureFunction
  // (this is NOT the index of FF into ScoreComponentCollection!!!)
  const size_t ffIndex = 0;

  // this is how to evaluate a single Hypothesis, see Hypothesis::EvaluateWhenApplied(float estimatedScore)

  Hypothesis *lastHypo = hypotheses.back();
  FFState const* prevState = lastHypo->GetPrevHypo() ? lastHypo->GetPrevHypo()->GetFFState(ffIndex) : NULL;
  FFState *curState = ff.EvaluateWhenApplied(*lastHypo, prevState, &scoreBreakdown);
  lastHypo->SetFFState(ffIndex, curState);

  // what happens if no state for previous Hypothesis computed yet?
  std::cout << "FF evaluated. Score: " << scoreBreakdown.GetScoreForProducer(&ff) << std::endl;

  // Moses::Factor::GetId() segfault.
}

BOOST_AUTO_TEST_SUITE_END()
