/****************************************************
 * Moses - factored phrase-based language decoder   *
 * Copyright (C) 2015 University of Edinburgh       *
 * Licensed under GNU LGPL Version 2.1, see COPYING *
 ****************************************************/

#include "MockHypothesis.h"

namespace Moses
{

const TargetPhrase& Hypothesis::GetCurrTargetPhrase() const { return *static_cast<const TargetPhrase*>(NULL); }

const Hypothesis* Hypothesis::GetPrevHypo() const { return NULL; }

}