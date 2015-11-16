/****************************************************
 * Moses - factored phrase-based language decoder   *
 * Copyright (C) 2015 University of Edinburgh       *
 * Licensed under GNU LGPL Version 2.1, see COPYING *
 ****************************************************/

#include "MockIncremental.h"
#include "lm/model.hh"

namespace Moses
{

namespace Incremental
{

// provide mock implementation for linking tests.
template<class Model> void Manager::LMCallback(const Model &model, const std::vector<lm::WordIndex> &words) {

}

template void Manager::LMCallback<lm::ngram::ProbingModel>(const lm::ngram::ProbingModel &model, const std::vector<lm::WordIndex> &words);
template void Manager::LMCallback<lm::ngram::RestProbingModel>(const lm::ngram::RestProbingModel &model, const std::vector<lm::WordIndex> &words);
template void Manager::LMCallback<lm::ngram::TrieModel>(const lm::ngram::TrieModel &model, const std::vector<lm::WordIndex> &words);
template void Manager::LMCallback<lm::ngram::QuantTrieModel>(const lm::ngram::QuantTrieModel &model, const std::vector<lm::WordIndex> &words);
template void Manager::LMCallback<lm::ngram::ArrayTrieModel>(const lm::ngram::ArrayTrieModel &model, const std::vector<lm::WordIndex> &words);
template void Manager::LMCallback<lm::ngram::QuantArrayTrieModel>(const lm::ngram::QuantArrayTrieModel &model, const std::vector<lm::WordIndex> &words);


}

}
