// vim:tabstop=2
#include "PhraseTableSADB.h"
//#include "moses/TranslationModel/CYKPlusParser/ChartRuleLookupManagerSkeleton.h"
//#include "moses/StaticData.h"
//#include "moses/TranslationTask.h"
#include "../PhraseBased/Manager.h"
#include "../PhraseBased/Sentence.h"
#include "../PhraseBased/TargetPhrases.h"
#include "../PhraseBased/TargetPhraseImpl.h"
#include "../MemPool.h"

#include <boost/lexical_cast.hpp>

#define ParseWord(w) (boost::lexical_cast<wid_t>((w)))

#ifdef VERBOSE
#undef VERBOSE
#endif
#define VERBOSE(l, x) ((void) 0)

using namespace std;
using namespace Moses;
using namespace mmt::sapt;

namespace Moses2 {
    PhraseTableSADB::PhraseTableSADB(size_t startInd, const std::string &line)
            : Moses2::PhraseTable(startInd, line) {
        this->m_numScores = mmt::sapt::kTranslationOptionScoreCount;
        ReadParameters();

        assert(m_input.size() == 1);
        assert(m_output.size() == 1);

        VERBOSE(3, GetScoreProducerDescription()
                << " PhraseTableSADB::PhraseTableSADB() m_filePath:|"
                << m_filePath << "|" << std::endl);
        VERBOSE(3, GetScoreProducerDescription()
                << " PhraseTableSADB::PhraseTableSADB() table-limit:|"
                << m_tableLimit << "|" << std::endl);
        VERBOSE(3, GetScoreProducerDescription()
                << " PhraseTableSADB::PhraseTableSADB() cache-size:|"
                << m_maxCacheSize << "|" << std::endl);
        VERBOSE(3, GetScoreProducerDescription()
                << " PhraseTableSADB::PhraseTableSADB() m_inputFactors:|"
                << m_inputFactors << "|" << std::endl);
        VERBOSE(3, GetScoreProducerDescription()
                << " PhraseTableSADB::PhraseTableSADB() m_outputFactors:|"
                << m_outputFactors << "|" << std::endl);
        VERBOSE(3, GetScoreProducerDescription()
                << " PhraseTableSADB::PhraseTableSADB() m_numScoreComponents:|"
                << m_numScoreComponents << "|" << std::endl);
        VERBOSE(3, GetScoreProducerDescription()
                << " PhraseTableSADB::PhraseTableSADB() m_input.size():|"
                << m_input.size() << "|" << std::endl);

        // caching for memory pt is pointless
        m_maxCacheSize = 0;
    }

    void PhraseTableSADB::Load(System &system) {
        //m_options = system.options;
        //SetFeaturesToApply();
        m_pt = new mmt::sapt::PhraseTable(m_modelPath, pt_options, nullptr); // TODO: StaticData::Instance().GetAligner()
    }

    PhraseTableSADB::~PhraseTableSADB() {
        delete m_pt;
    }

    inline vector<wid_t> PhraseTableSADB::ParsePhrase(const SubPhrase<Moses2::Word> &phrase) const {
        vector<wid_t> result(phrase.GetSize());

        for (size_t i = 0; i < phrase.GetSize(); i++) {
            result[i] = ParseWord(phrase[i].GetString(m_input));
        }

        return result;
    }

    TargetPhrases *
    PhraseTableSADB::MakeTargetPhrases(const Manager &mgr, const Phrase<Moses2::Word> &sourcePhrase,
                                       const vector<mmt::sapt::TranslationOption> &options) const
    {
        auto &pool = mgr.GetPool();
        FactorCollection &vocab = mgr.system.GetVocab();
        TargetPhrases *tps = new (pool.Allocate<TargetPhrases>()) TargetPhrases(pool, options.size());

        //transform the SAPT translation Options into Moses Target Phrases
        for (auto target_options_it = options.begin();
             target_options_it != options.end(); ++target_options_it)
        {
            const vector<wid_t> &tpw = target_options_it->targetPhrase;
            TargetPhraseImpl *tp = new TargetPhraseImpl(pool, *this, mgr.system, tpw.size());

            // words
            for (size_t i = 0; i < tpw.size(); i++) {
                Moses2::Word &word = (*tp)[i];
                word.CreateFromString(vocab, mgr.system, to_string(tpw[i]));
            }

            // align
            std::set<std::pair<size_t, size_t> > aln;
            for (auto alignment_it = target_options_it->alignment.begin();
                 alignment_it != target_options_it->alignment.end(); ++alignment_it)
            {
                aln.insert(std::make_pair(size_t(alignment_it->first), size_t(alignment_it->second)));
            }
            tp->SetAlignTerm(aln);

            // scores
            Scores &scores = tp->GetScores();
            scores.Assign(mgr.system, *this, target_options_it->scores);

            mgr.system.featureFunctions.EvaluateInIsolation(pool, mgr.system, sourcePhrase, *tp);

            tps->AddTargetPhrase(*tp);
        }

        tps->SortAndPrune(m_tableLimit);
        mgr.system.featureFunctions.EvaluateAfterTablePruning(pool, *tps, sourcePhrase);

        return tps;
    }

    void PhraseTableSADB::SetParameter(const std::string &key, const std::string &value) {

        if (key == "path") {
            m_modelPath = Scan<std::string>(value);
            VERBOSE(3, "m_modelPath:" << m_modelPath << std::endl);
        } else if (key == "sample-limit") {
            pt_options.samples = Scan<int>(value);
            VERBOSE(3, "pt_options.sample:" << pt_options.samples << std::endl);
        } else {
            PhraseTable::SetParameter(key, value);
        }
    }

    void PhraseTableSADB::Lookup(const Manager &mgr, InputPathsBase &inputPaths) const
    {
        auto &sent = dynamic_cast<const Sentence &>(mgr.GetInput());
        SubPhrase<Moses2::Word> sourceSentence = sent.GetSubPhrase(0, sent.GetSize());

        context_t *context = t_context_vec.get();

        vector<wid_t> sentence = ParsePhrase(sourceSentence);
        mmt::sapt::translation_table_t ttable = m_pt->GetAllTranslationOptions(sentence, context);

        for(InputPathBase *pathBase : inputPaths) {
          InputPath *path = static_cast<InputPath*>(pathBase);
          vector<wid_t> phrase = ParsePhrase(path->subPhrase);

          auto options = ttable.find(phrase);
          if (options != ttable.end()) {
            auto tps = MakeTargetPhrases(mgr, path->subPhrase, options->second);
            path->AddTargetPhrases(*this, tps);
          }
        }
    }

    TargetPhrases *PhraseTableSADB::Lookup(const Manager &mgr, MemPool &pool,
        InputPath &inputPath) const
    {
        // performance wise, this is not wise.
        // Instead, use Lookup(InputPathsBase &) for a whole sentence at a time.
        UTIL_THROW2("Not implemented");
        return nullptr;
    }

    // scfg
    void PhraseTableSADB::InitActiveChart(
        MemPool &pool,
        const SCFG::Manager &mgr,
        SCFG::InputPath &path) const
    {
        UTIL_THROW2("Not implemented");
    }

    void PhraseTableSADB::Lookup(
        MemPool &pool,
        const SCFG::Manager &mgr,
        size_t maxChartSpan,
        const SCFG::Stacks &stacks,
        SCFG::InputPath &path) const
    {
        UTIL_THROW2("Not implemented");
    }

    void PhraseTableSADB::LookupGivenNode(
        MemPool &pool,
        const SCFG::Manager &mgr,
        const SCFG::ActiveChartEntry &prevEntry,
        const SCFG::Word &wordSought,
        const Moses2::Hypotheses *hypos,
        const Moses2::Range &subPhraseRange,
        SCFG::InputPath &outPath) const
    {
        UTIL_THROW2("Not implemented");
    }



    //TO_STRING_BODY(PhraseTableSADB);

// friend
    ostream &operator<<(ostream &out, const PhraseTableSADB &phraseDict) {
        return out;
    }

}
