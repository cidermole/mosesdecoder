/*
 * SkeletonStatefulFF.h
 *
 *  Created on: 27 Oct 2015
 *      Author: hieu
 */

#pragma once

#include "StatelessFeatureFunction.h"

namespace Moses2
{

class SkeletonStatelessFF: public StatelessFeatureFunction
{
public:
  SkeletonStatelessFF(size_t startInd, const std::string &line);
  virtual ~SkeletonStatelessFF();

  virtual void
  EvaluateInIsolation(MemPool &pool, const System &system, const Phrase<Moses2::Word> &source,
      const TargetPhraseImpl &targetPhrase, Scores &scores,
      SCORE &estimatedScore) const;

  virtual void
  EvaluateInIsolation(MemPool &pool, const System &system, const Phrase<SCFG::Word> &source,
      const TargetPhrase<SCFG::Word> &targetPhrase, Scores &scores,
      SCORE &estimatedScore) const;

};

}

