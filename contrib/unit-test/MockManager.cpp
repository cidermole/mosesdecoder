/****************************************************
 * Moses - factored phrase-based language decoder   *
 * Copyright (C) 2015 University of Edinburgh       *
 * Licensed under GNU LGPL Version 2.1, see COPYING *
 ****************************************************/

#include "MockManager.h"

namespace Moses
{

SentenceStats& Manager::GetSentenceStats() const
{
  assert(false && "not implemented: MockManager.cpp Manager::GetSentenceStats()");
  return *m_sentenceStats;
}

AllOptions const &
BaseManager::
options() const {
  //return GetTtask()->options();
  assert(false && "not implemented: MockManager.cpp BaseManager::options()");
  return AllOptions();
}

}
