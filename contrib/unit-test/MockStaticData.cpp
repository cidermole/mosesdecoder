/****************************************************
 * Moses - factored phrase-based language decoder   *
 * Copyright (C) 2015 University of Edinburgh       *
 * Licensed under GNU LGPL Version 2.1, see COPYING *
 ****************************************************/

#include "MockStaticData.h"

namespace Moses
{

StaticData StaticData::s_instance;

StaticData::StaticData() {
  m_factorDelimiter = "|";
}
StaticData::~StaticData() {}

FeatureRegistry::FeatureRegistry() {}
FeatureRegistry::~FeatureRegistry() {}

PhrasePropertyFactory::PhrasePropertyFactory() {}
PhrasePropertyFactory::~PhrasePropertyFactory() {}

ContextParameters::ContextParameters() {}

InputOptions::InputOptions() {}

}
