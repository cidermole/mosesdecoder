// $Id$

/***********************************************************************
Moses - factored phrase-based language decoder
Copyright (C) 2006 University of Edinburgh

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation; either
version 2.1 of the License, or (at your option) any later version.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this library; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
***********************************************************************/

#include <limits>
#include <iostream>
#include <fstream>
#include <algorithm>

#include "MMTInterpolatedLM.h"

#include "../Phrase.h"
#include "../Scores.h"
#include "../System.h"
#include "../PhraseBased/Hypothesis.h"
#include "../PhraseBased/Manager.h"
#include "../PhraseBased/TargetPhraseImpl.h"

#define ParseWord(w) (boost::lexical_cast<wid_t>((w)))

#ifdef VERBOSE
#undef VERBOSE
#endif
#define VERBOSE(l, x) ((void) 0)

using namespace std;
using namespace Moses;

namespace Moses2 {

class ILMState : public FFState {
  friend class MMTInterpolatedLM;

  friend ostream &operator<<(ostream &out, const ILMState &obj);

public:
  size_t hash() const {
      return state->hash();
  }

  bool operator==(const FFState &o) const {
      const ILMState &other = static_cast<const ILMState &>(o);
      return (*state == *(other.state));
  }

  ILMState(HistoryKey *st) : state(st) {}

  ILMState(shared_ptr<HistoryKey> &st) : state(st) {}

  virtual std::string ToString() const override {
      stringstream ss;
      ss << state;
      return ss.str();
  }

private:
  shared_ptr<HistoryKey> state;
};

// friend
ostream &operator<<(ostream &out, const ILMState &obj) {
    out << " obj.state:|" << obj.state << "|";
    return out;
}

MMTInterpolatedLM::MMTInterpolatedLM(size_t startInd, const std::string &line) : StatefulFeatureFunction(startInd, line), m_nGramOrder(0), m_enableOOVFeature(false) {
    ReadParameters();

    VERBOSE(3, GetScoreProducerDescription()
        << " MMTInterpolatedLM::MMTInterpolatedLM() m_nGramOrder:|"
        << m_nGramOrder << "|" << std::endl);
    VERBOSE(3, GetScoreProducerDescription()
        << " MMTInterpolatedLM::MMTInterpolatedLM() m_modelPath:|"
        << m_modelPath << "|" << std::endl);
    VERBOSE(3, GetScoreProducerDescription()
        << " MMTInterpolatedLM::MMTInterpolatedLM() m_factorType:|"
        << m_factorType << "|" << std::endl);

}

MMTInterpolatedLM::~MMTInterpolatedLM() {
    delete m_lm;
}

void MMTInterpolatedLM::Load(System &system) {
    if (m_nGramOrder == 0)
        m_nGramOrder = lm_options.order;
    else
        m_nGramOrder = min(m_nGramOrder, (size_t) lm_options.order);

    m_lm = new InterpolatedLM(m_modelPath, lm_options);
}

FFState *MMTInterpolatedLM::BlankState(MemPool &pool, const System &sys) const {
    vector<wid_t> phrase(1);
    phrase[0] = kVocabularyStartSymbol;

    /*
     * TODO: avoid putting non-POD data into state
     *
     * moses2 is working with memory pools: you allocate state objects in one blob
     * you never actually delete them, since all are PODs
     *
     * but HistoryKey contains, eventually, a vector<wid_t>
     * ideally, that would all be pool-allocated / struct with array of fixed number of words
     *
     * options:
     * 1) create an interface to tell the MemPool our state size
     * 2) use compile time constants to fix our state size
     *
     * -- David <git@abanbytes.eu>
     *
     */
    return new(pool.Allocate<ILMState>()) ILMState(m_lm->MakeHistoryKey(phrase));
}

void MMTInterpolatedLM::EmptyHypothesisState(FFState &state, const ManagerBase &mgr,
                                             const InputType &input, const Hypothesis &hypo) const
{
    vector<wid_t> phrase(1);
    phrase[0] = kVocabularyStartSymbol;
    ILMState &ourState = static_cast<ILMState &>(state);
    // TODO: avoid putting non-POD data into state -- see above comment in BlankState()
    ourState.state.reset(m_lm->MakeHistoryKey(phrase));
}

void
MMTInterpolatedLM::TransformPhrase(const Phrase<Moses2::Word> &phrase, std::vector<wid_t> &phrase_vec,
                                   const size_t startGaps,
                                   const size_t endGaps) const {
    for (size_t i = 0; i < startGaps; ++i) {
        phrase_vec.push_back(kVocabularyStartSymbol); //insert start symbol
    }
    for (size_t i = 0; i < phrase.GetSize(); ++i) {
        wid_t id = ParseWord(phrase[i][m_factorType]->GetString().as_string());
        phrase_vec.push_back(id);
    }
    for (size_t i = 0; i < endGaps; ++i) {
        phrase_vec.push_back(kVocabularyEndSymbol); //insert end symbol
    }
}

void
MMTInterpolatedLM::SetWordVector(const Hypothesis &hypo, std::vector<wid_t> &phrase_vec, const size_t startGaps,
                                 const size_t endGaps, const size_t from, const size_t to) const {
    for (size_t i = 0; i < startGaps; ++i) {
        phrase_vec.push_back(kVocabularyStartSymbol); //insert start symbol
    }

    for (size_t position = from; position < to; ++position) {
        phrase_vec.push_back(ParseWord(hypo.GetWord(position)[m_factorType]->GetString().as_string()));
    }

    for (size_t i = 0; i < endGaps; ++i) {
        phrase_vec.push_back(kVocabularyEndSymbol); //insert end symbol
    }
}

void
MMTInterpolatedLM::CalcScore(const Phrase<Moses2::Word> &phrase, float &fullScore, float &ngramScore,
                             size_t &oovCount) const {
    VERBOSE(3,
            "void MMTInterpolatedLM::CalcScore(const Phrase &phrase, ...) const START phrase:|" << phrase << "|"
                                                                                                <<
                                                                                                std::endl);

    fullScore = 0;
    ngramScore = 0;
    oovCount = 0;

    if (phrase.GetSize() == 0) return;

    std::vector<wid_t> phrase_vec;
    TransformPhrase(phrase, phrase_vec, 0, 0);

    size_t boundary = m_nGramOrder - 1;

    context_t *context_vec = t_context_vec.get();
    if (context_vec == nullptr) {
        VERBOSE(3, "void MMTInterpolatedLM::CalcScore(const Phrase &phrase, ...) const context is null"
            << std::endl);
    } else if (context_vec->empty()) {
        VERBOSE(3, "void MMTInterpolatedLM::CalcScore(const Phrase &phrase, ...) const context is empty"
            << std::endl);
    } else {
        VERBOSE(3,
                "void MMTInterpolatedLM::CalcScore(const Phrase &phrase, ...) const context is not empty not null, size:|"
                    <<
                    context_vec->size() << "|" << std::endl);
    }

    CachedLM *lm = t_cached_lm.get();

    HistoryKey *cursorHistoryKey = lm->MakeEmptyHistoryKey();
    for (size_t position = 0; position < phrase_vec.size(); ++position) {
        HistoryKey *outHistoryKey = NULL;
        double prob = lm->ComputeProbability(phrase_vec.at(position), cursorHistoryKey, context_vec, &outHistoryKey);

        delete cursorHistoryKey;
        cursorHistoryKey = outHistoryKey;

        fullScore += prob;
        if (position >= boundary) {
            ngramScore += prob;
        }
    }

    delete cursorHistoryKey;

    if (m_enableOOVFeature) {
        for (size_t position = 0; position < phrase_vec.size(); ++position) {
            // verifying whether the actual word is an OOV, and in case increase the oovCount
            if (lm->IsOOV(context_vec, phrase_vec.at(position)))
                ++oovCount;
        }
    }
}

void
MMTInterpolatedLM::EvaluateInIsolation(MemPool &pool, const System &system,
                                       const Phrase<Moses2::Word> &source,
                                       const TargetPhraseImpl &targetPhrase, Scores &scores,
                                       SCORE &estimatedScore) const
{
    VERBOSE(2, "void LanguageModel::EvaluateInIsolation(const Phrase &source, const TargetPhrase &targetPhrase, ...)"
        << std::endl);
    // contains factors used by this LM
    float fullScore, nGramScore;
    size_t oovCount;

    VERBOSE(2, "targetPhrase:|" << targetPhrase << "|" << std::endl);
    VERBOSE(2, "pthread_self():" << pthread_self() << endl);

    CalcScore(targetPhrase, fullScore, nGramScore, oovCount);

    float estimateScore = fullScore - nGramScore;

    if (m_enableOOVFeature) {
        float score[2], estimateScores[2];
        score[0] = nGramScore;
        score[1] = oovCount;
        scores.PlusEquals(system, *this, score);

        estimateScores[0] = estimateScore;
        estimateScores[1] = 0;
        SCORE weightedScore = Scores::CalcWeightedScore(system, *this,
                                                        estimateScores);
        estimatedScore += weightedScore;
    } else {
        scores.PlusEquals(system, *this, nGramScore);

        SCORE weightedScore = Scores::CalcWeightedScore(system, *this,
                                                        estimateScore);
        estimatedScore += weightedScore;
    }

    VERBOSE(2, "CalcScore of targetPhrase:|" << targetPhrase << "|: ngr=" << nGramScore << " est=" << estimateScore
                                             << std::endl);
}

void MMTInterpolatedLM::EvaluateWhenApplied(const ManagerBase &mgr,
                                            const Hypothesis &hypo, const FFState &prevState, Scores &scores,
                                            FFState &state) const {
    VERBOSE(4,
            "FFState* MMTInterpolatedLM::EvaluateWhenApplied(const Hypothesis &hypo, const FFState *ps, ScoreComponentCollection *out) const"
                << std::endl);

    if (hypo.GetTargetPhrase().GetSize() == 0) {
        //ILMState &inState = const_cast<ILMState &>(static_cast<const ILMState &>(prevState));
        const ILMState &inState = static_cast<const ILMState &>(prevState);
        ILMState &outState = static_cast<ILMState &>(state);
        outState.state = inState.state;
    }

    //[begin, end) in STL-like fashion.
    const int begin = (const int) hypo.GetCurrTargetWordsRange().GetStartPos();
    const int end = (const int) (hypo.GetCurrTargetWordsRange().GetEndPos() + 1);

    int adjust_end = (int) (begin + m_nGramOrder - 1);

    if (adjust_end > end) {
        adjust_end = end;
    }

    std::vector<wid_t> phrase_vec;
    SetWordVector(hypo, phrase_vec, 0, 0, begin, adjust_end);

    context_t *context_vec = t_context_vec.get();
    if (context_vec == nullptr) {
        VERBOSE(4, "void MMTInterpolatedLM::EvaluateWhenApplied(const Phrase &phrase, ...) const context is null"
            << std::endl);
    } else if (context_vec->empty()) {
        VERBOSE(4,
                "void MMTInterpolatedLM::EvaluateWhenApplied(const Phrase &phrase, ...) const context is empty"
                    << std::endl);
    } else {
        VERBOSE(4,
                "void MMTInterpolatedLM::EvaluateWhenApplied(const Phrase &phrase, ...) const context is not empty not null, size:|"
                    <<
                    context_vec->size() << "|" << std::endl);
    }

    CachedLM *lm = t_cached_lm.get();
    double score = 0.0;

    const ILMState &inState = static_cast<const ILMState &>(prevState);
    HistoryKey *initialState = inState.state.get();
    HistoryKey *cursorHistoryKey = NULL;

    for (size_t position = 0; position < phrase_vec.size(); ++position) {
        HistoryKey *outHistoryKey;
        double prob = lm->ComputeProbability(phrase_vec.at(position),
                                             cursorHistoryKey ? cursorHistoryKey : initialState,
                                             context_vec, &outHistoryKey);
        if (cursorHistoryKey)
            delete cursorHistoryKey;
        cursorHistoryKey = outHistoryKey;
        score += prob;
    }

    // adding probability of having sentenceEnd symbol, after this phrase;
    // this could happen only when all source words are covered
    if (hypo.GetBitmap().IsComplete()) {
        std::vector<wid_t> ngram_vec;
        int adjust_begin = end - ((int) m_nGramOrder - 1);
        size_t startGaps = 0;
        if (adjust_begin < 0) {
            startGaps = 1;
            adjust_begin = 0;
        }

        //if the phrase is too short, one StartSentenceSymbol (see startGaps) is added
        SetWordVector(hypo, ngram_vec, startGaps, 0, adjust_begin, end);

        HistoryKey *outHistoryKey;

        HistoryKey *tmpHistoryKey = lm->MakeHistoryKey(ngram_vec);
        score += lm->ComputeProbability(kVocabularyEndSymbol, tmpHistoryKey, t_context_vec.get(), &outHistoryKey);
        delete tmpHistoryKey;

        if (cursorHistoryKey)
            delete cursorHistoryKey;
        cursorHistoryKey = outHistoryKey;
    } else {
        // need to set the LM state
        if (adjust_end < end) { // the LMstate of this target phrase refers to the last m_lmtb_size-1 words

            std::vector<wid_t> ngram_vec;
            int adjust_begin = end - ((int) m_nGramOrder - 1);
            if (adjust_begin < 0) {
                adjust_begin = 0;
            }

            // because of the size of the phrase (Larger then m_ngram_order) it is not possible that
            // the relevant words for the state contain the StartSentenceSymbol
            SetWordVector(hypo, ngram_vec, 0, 0, adjust_begin, end);

            if (cursorHistoryKey)
                delete cursorHistoryKey;
            cursorHistoryKey = lm->MakeHistoryKey(ngram_vec);
        }
    }

    scores.PlusEquals(mgr.system, *this, score); // score is already expressed as natural log probability

    ILMState &outState = static_cast<ILMState &>(state);
    outState.state.reset(cursorHistoryKey);
}

void MMTInterpolatedLM::InitializeForInput(const Manager &mgr) const {
    // moses2 keeps const pointers to feature functions, so we must be 'void() const' unless you change everything else.
    const_cast<MMTInterpolatedLM *>(this)->InitializeForInput(mgr);
}

void MMTInterpolatedLM::InitializeForInput(const Manager &mgr) {
    // we assume here that translation is run in one single thread for each ttask
    // (no parallelization at a finer granularity involving MMT InterpolatedLM)

    // This function is called prior to actual translation and allows the class
    // to set up thread-specific information such as context weights

    // DO NOT modify members of 'this' here. We are being called from different
    // threads, and there is no locking here.

    /*
    SPTR<ContextScope> const &scope = ttask->GetScope();
    SPTR<weightmap_t const> weights = scope->GetContextWeights();
    */
    SPTR<const weightmap_t> weights; // TODO get context weights

    if (weights) {
        context_t *context_vec = new context_t;

        for (weightmap_t::const_iterator it = weights->begin(); it != weights->end(); ++it) {
            context_vec->push_back(cscore_t(ParseWord(it->first), it->second));
        }

        m_lm->NormalizeContext(context_vec);
        t_context_vec.reset(context_vec);
    }

    t_cached_lm.reset(new CachedLM(m_lm, 5));
}

void MMTInterpolatedLM::CleanUpAfterSentenceProcessing() const {
    // moses2 keeps const pointers to feature functions, so we must be 'void() const' unless you change everything else.
    const_cast<MMTInterpolatedLM *>(this)->CleanUpAfterSentenceProcessing();
}

void MMTInterpolatedLM::CleanUpAfterSentenceProcessing() {
    t_context_vec.reset();
    t_cached_lm.reset();
}

void MMTInterpolatedLM::SetParameter(const std::string &key, const std::string &value) {
    if (key == "factor") {
        m_factorType = boost::lexical_cast<FactorType>(value);
    } else if (key == "path") {
        m_modelPath = value;
        VERBOSE(3, "m_modelPath:" << m_modelPath << std::endl);
    } else if (key == "order") {
        m_nGramOrder = Scan<size_t>(value);
    } else if (key == "adaptivity-ratio") {
        lm_options.adaptivity_ratio = Scan<float>(value);
        VERBOSE(3, "lm_options.adaptivity_ratio:" << lm_options.adaptivity_ratio << std::endl);
    } else {
        StatefulFeatureFunction::SetParameter(key, value);
    }
}

} // namespace Moses2
