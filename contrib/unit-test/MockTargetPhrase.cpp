/****************************************************
 * Moses - factored phrase-based language decoder   *
 * Copyright (C) 2015 University of Edinburgh       *
 * Licensed under GNU LGPL Version 2.1, see COPYING *
 ****************************************************/

#include "MockTargetPhrase.h"
#include "moses/StaticData.h"

using namespace std;

namespace Moses
{

TargetPhrase::TargetPhrase(const PhraseDictionary *pt)
    :Phrase()
    , m_fullScore(0.0)
    , m_futureScore(0.0)
    , m_alignTerm(NULL)
    , m_alignNonTerm(NULL)
    , m_lhsTarget(NULL)
    , m_ruleSource(NULL)
    , m_container(pt)
{
}

TargetPhrase::TargetPhrase(const TargetPhrase &copy)
    : Phrase(copy)
    , m_cached_scores(copy.m_cached_scores)
    , m_fullScore(copy.m_fullScore)
    , m_futureScore(copy.m_futureScore)
    , m_scoreBreakdown(copy.m_scoreBreakdown)
    , m_alignTerm(copy.m_alignTerm)
    , m_alignNonTerm(copy.m_alignNonTerm)
    , m_properties(copy.m_properties)
    , m_scope(copy.m_scope)
    , m_container(copy.m_container)
{
  if (copy.m_lhsTarget) {
    m_lhsTarget = new Word(*copy.m_lhsTarget);
  } else {
    m_lhsTarget = NULL;
  }

  if (copy.m_ruleSource) {
    m_ruleSource = new Phrase(*copy.m_ruleSource);
  } else {
    m_ruleSource = NULL;
  }
}

void
TargetPhrase::
SetExtraScores(FeatureFunction const* ff,
               boost::shared_ptr<Scores> const& s)
{
  m_cached_scores[ff] = s;
}

Scores const*
TargetPhrase::
GetExtraScores(FeatureFunction const* ff) const
{
  ScoreCache_t::const_iterator m = m_cached_scores.find(ff);
  return m != m_cached_scores.end() ? m->second.get() : NULL;
}

void TargetPhrase::EvaluateWithSourceContext(const InputType &input, const InputPath &inputPath)
{
  const std::vector<FeatureFunction*> &ffs = FeatureFunction::GetFeatureFunctions();
  const StaticData &staticData = StaticData::Instance();
  ScoreComponentCollection futureScoreBreakdown;
  for (size_t i = 0; i < ffs.size(); ++i) {
    const FeatureFunction &ff = *ffs[i];
    if (! staticData.IsFeatureFunctionIgnored( ff )) {
      ff.EvaluateWithSourceContext(input, inputPath, *this, NULL, m_scoreBreakdown, &futureScoreBreakdown);
    }
  }
  float weightedScore = m_scoreBreakdown.GetWeightedScore();
  m_futureScore += futureScoreBreakdown.GetWeightedScore();
  m_fullScore = weightedScore + m_futureScore;
}

TargetPhrase::~TargetPhrase()
{
  //cerr << "m_lhsTarget=" << m_lhsTarget << endl;

  delete m_lhsTarget;
  delete m_ruleSource;
}

bool TargetPhrase::HasScope() const
{
  return !m_scope.expired(); // should actually never happen
}

SPTR<ContextScope> TargetPhrase::GetScope() const
{
  return m_scope.lock();
}


std::ostream& operator<<(std::ostream& os, const TargetPhrase& tp)
{
  if (tp.m_lhsTarget) {
    os << *tp.m_lhsTarget<< " -> ";
  }

  os << static_cast<const Phrase&>(tp) << ":" << flush;
  os << tp.GetAlignNonTerm() << flush;
  os << ": term=" << tp.GetAlignTerm() << flush;
  os << ": nonterm=" << tp.GetAlignNonTerm() << flush;
  os << ": c=" << tp.m_fullScore << flush;
  os << " " << tp.m_scoreBreakdown << flush;

  const Phrase *sourcePhrase = tp.GetRuleSource();
  if (sourcePhrase) {
    os << " sourcePhrase=" << *sourcePhrase << flush;
  }

  /*
  if (tp.m_properties.size()) {
    os << " properties: " << flush;

    TargetPhrase::Properties::const_iterator iter;
    for (iter = tp.m_properties.begin(); iter != tp.m_properties.end(); ++iter) {
      const string &key = iter->first;
      const PhraseProperty *prop = iter->second.get();
      assert(prop);

      os << key << "=" << *prop << " ";
    }
  }
  */

  return os;
}

// for TranslationOption (usually implemented in Range)
std::ostream& operator << (std::ostream& out, const Range& range)
{
  out << "[" << range.m_startPos << ".." << range.m_endPos << "]";
  return out;
}


}