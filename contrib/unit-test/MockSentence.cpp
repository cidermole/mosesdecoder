/****************************************************
 * Moses - factored phrase-based language decoder   *
 * Copyright (C) 2015 University of Edinburgh       *
 * Licensed under GNU LGPL Version 2.1, see COPYING *
 ****************************************************/

#include "MockSentence.h"

using namespace std;

namespace Moses
{

Sentence::
Sentence() : Phrase(0) , InputType()
{
  /*
  const StaticData& SD = StaticData::Instance();
  if (SD.IsSyntax())
    m_defaultLabelSet.insert(SD.GetInputDefaultNonTerminal());
  */
}


Sentence::
Sentence(size_t const transId,
         string const& stext,
         AllOptions const& opts,
         vector<FactorType> const* IFO)
    : InputType(transId)
{
  /*
  if (IFO) init(stext, *IFO, opts);
  else init(stext, StaticData::Instance().GetInputFactorOrder(), opts);
  */
}

Sentence::~Sentence() {}

int
Sentence::
Read(std::istream& in,
     const std::vector<FactorType>& factorOrder,
     AllOptions const& opts)
{
  assert(false);
  return 0;
}

void Sentence::Print(std::ostream& out) const
{
  out<<*static_cast<Phrase const*>(this);
}

TranslationOptionCollection*
Sentence::
CreateTranslationOptionCollection(ttasksptr const& ttask) const
{
  assert(false);
  return NULL;
}

void
Sentence::
CreateFromString(vector<FactorType> const& FOrder, string const& phraseString)
{
  Phrase::CreateFromString(Input, FOrder, phraseString, NULL);
}


// why these below?

std::vector <ChartTranslationOptions*> Sentence::GetXmlChartTranslationOptions(AllOptions const& opts) const
{
  assert(false);
  return std::vector <ChartTranslationOptions*>();
}

}