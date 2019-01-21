#include "Analysis/ProgramPartitionAnalysis.h"

#include "Analysis/Partitioner.h"
#include "Analysis/CallGraph.h"
#include "Utils/Logger.h"
#include "Utils/Statistics.h"
#include "Utils/AnnotationParser.h"
#include "Utils/JsonAnnotationParser.h"
#include "Utils/ModuleAnnotationParser.h"
#include "Optimization/PartitionOptimizer.h"

#include "PDG/Passes/PDGBuildPasses.h"

#include "llvm/Analysis/CallGraph.h"
#include "llvm/Analysis/LoopInfo.h"
#include "llvm/Analysis/PostDominators.h"
#include "llvm/IR/Dominators.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Constants.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/raw_ostream.h"

#include "llvm/IR/LegacyPassManager.h"
#include "llvm/PassRegistry.h"
#include "llvm/Transforms/IPO/PassManagerBuilder.h"

#include <algorithm>
#include <fstream>

namespace vazgen {

namespace {

auto getOptimizations(const std::string& optName, Logger& logger)
{
    PartitionOptimizer::Optimizations opts;
    if (optName == "local") {
        opts.push_back(PartitionOptimizer::FUNCTIONS_MOVE_TO);
        opts.push_back(PartitionOptimizer::GLOBALS_MOVE_TO);
        opts.push_back(PartitionOptimizer::DUPLICATE_FUNCTIONS);
    } else if (optName == "function_move") {
        opts.push_back(PartitionOptimizer::FUNCTIONS_MOVE_TO);
    } else if (optName == "global_move") {
        opts.push_back(PartitionOptimizer::GLOBALS_MOVE_TO);
    } else if (optName == "function_duplicate") {
        opts.push_back(PartitionOptimizer::DUPLICATE_FUNCTIONS);
    } else if (optName == "kl") {
        opts.push_back(PartitionOptimizer::KERNIGHAN_LIN);
    } else {
        logger.error("No optimization with name " + optName);
    }
    return opts;
}

}


class ProgramPartition::PartitionStatistics : public Statistics
{
public:
    PartitionStatistics(std::ofstream& strm,
                        const Partition& partition,
                        llvm::Module& M);

    void report() final;

private:
    const Partition& m_partition;
    llvm::Module& m_module;
}; // class PartitionStatistics

ProgramPartition::PartitionStatistics::PartitionStatistics(std::ofstream& strm,
                                                           const Partition& partition,
                                                           llvm::Module& M)
    : Statistics(strm, Statistics::JSON)
    , m_partition(partition)
    , m_module(M)
{
}

void ProgramPartition::PartitionStatistics::report()
{
    std::vector<std::string> partitionFs;
    partitionFs.reserve(m_partition.getPartition().size());
    std::transform(m_partition.getPartition().begin(), m_partition.getPartition().end(), std::back_inserter(partitionFs),
            [] (llvm::Function* F) { return F->getName().str();});
    write_entry({"program_partition", "partitioned_functions"}, partitionFs);
    write_entry({"program_partition", "partition_size"}, (unsigned) partitionFs.size());
    double partition_portion = (m_partition.getPartition().size() * 100.0) / m_module.size();
    write_entry({"program_partition", "partition%"}, partition_portion);

    std::vector<std::string> staticAnalysisFs;
    staticAnalysisFs.reserve(m_partition.getRelatedFunctions().size());
    std::transform(m_partition.getRelatedFunctions().begin(), m_partition.getRelatedFunctions().end(), std::back_inserter(staticAnalysisFs),
            [] (const auto& pair) { return pair.first->getName().str() + std::to_string(pair.second);});
    write_entry({"program_partition", "security_related_functions"}, staticAnalysisFs);
    write_entry({"program_partition", "security_related_functions_size"}, (unsigned) staticAnalysisFs.size());
    double security_related_portion = (m_partition.getRelatedFunctions().size() * 100.0) / m_module.size();
    write_entry({"program_partition", "security_related%"}, security_related_portion);

    std::vector<std::string> inInterface;
    inInterface.reserve(m_partition.getInInterface().size());
    std::transform(m_partition.getInInterface().begin(), m_partition.getInInterface().end(), std::back_inserter(inInterface),
            [] (llvm::Function* F) { return F->getName().str();});
    write_entry({"program_partition", "in_interface"}, inInterface);
    write_entry({"program_partition", "in_interface_size"}, (unsigned) inInterface.size());
    double inInterface_portion = (m_partition.getInInterface().size() * 100.0) / m_module.size();
    write_entry({"program_partition", "in_interface%"}, inInterface_portion);

    std::vector<std::string> outInterface;
    outInterface.reserve(m_partition.getOutInterface().size());
    std::transform(m_partition.getOutInterface().begin(), m_partition.getOutInterface().end(), std::back_inserter(outInterface),
            [] (llvm::Function* F) { return F->getName().str();});
    write_entry({"program_partition", "out_interface"}, outInterface);
    write_entry({"program_partition", "out_interface_size"}, (unsigned) outInterface.size());
    double outInterface_portion = (m_partition.getOutInterface().size() * 100.0) / m_module.size();
    write_entry({"program_partition", "out_interface%"}, outInterface_portion);

    std::vector<std::string> globals;
    globals.reserve(m_partition.getGlobals().size());
    std::transform(m_partition.getGlobals().begin(), m_partition.getGlobals().end(), std::back_inserter(globals),
            [] (llvm::GlobalVariable* global) { return global->getName().str();});
    write_entry({"program_partition", "globals"}, globals);
    write_entry({"program_partition", "globals_size"}, (unsigned) globals.size());
    double globals_portion = (m_partition.getGlobals().size() * 100.0) / m_module.getGlobalList().size();
    write_entry({"program_partition", "globals%"}, globals_portion);

    flush();
}

ProgramPartition::ProgramPartition(llvm::Module& M,
                                   PDGType pdg,
                                   const llvm::CallGraph& callgraph,
                                   Logger& logger)
    : m_module(M)
    , m_pdg(pdg)
    , m_callgraph(callgraph)
    , m_logger(logger)
{
}

void ProgramPartition::setLoopInfoGetter(const LoopInfoGetter& loopInfoGetter)
{
    m_loopInfoGetter = loopInfoGetter;
}

void ProgramPartition::partition(const Annotations& annotations)
{
    Partitioner partitioner(m_module, m_pdg, m_logger);
    partitioner.partition(annotations);
    m_securePartition = partitioner.getSecurePartition();
    m_insecurePartition = partitioner.getInsecurePartition();
}

void ProgramPartition::optimize(auto optimizations)
{
    CallGraph callgraph(m_callgraph);
    callgraph.assignWeights(m_securePartition, m_insecurePartition, m_pdg.get(), m_loopInfoGetter);
    PartitionOptimizer optimizer(m_securePartition, m_insecurePartition, m_pdg, callgraph, m_logger);
    optimizer.setLoopInfoGetter(m_loopInfoGetter);
    optimizer.run(optimizations);
}

void ProgramPartition::dump(const std::string& outFile) const
{
    const auto& partitionFs = m_securePartition.getPartition();
    if (!outFile.empty()) {
        std::ofstream ostr(outFile);
        ostr << "Partition functions-----------\n";
        for (auto& partitionF : partitionFs) {
            ostr << partitionF->getName().str() << "\n";
        }
        ostr << "Static analyser output--------\n";
        for (auto& [function, level] : m_securePartition.getRelatedFunctions()) {
            ostr << function->getName().str() << " level " << level << "\n";
        }
        ostr.close();
        return;
    }
    llvm::dbgs() << "Partition of Module " << m_module.getName() << "\n";
    for (auto F : partitionFs) {
        llvm::dbgs() << "   " << F->getName() << "\n";
    }
}

void ProgramPartition::dumpStats(const std::string& statsFile) const
{
    std::ofstream strm;
    if (statsFile.empty()) {
        strm.open("partition_stats.json");
    } else {
        strm.open(statsFile);
    }
    PartitionStatistics stats(strm, m_securePartition, m_module);
    stats.report();
}

llvm::cl::opt<std::string> JsonAnnotations(
    "json-annotations",
    llvm::cl::desc("Json file containing annotations"),
    llvm::cl::value_desc("annotation"));

llvm::cl::opt<std::string> Outfile(
    "outfile",
    llvm::cl::desc("Out file to write partitioning information"),
    llvm::cl::value_desc("outfile name"));

llvm::cl::opt<bool> Stats(
    "partition-stats",
    llvm::cl::desc("Dump partition stats"),
    llvm::cl::value_desc("flag to dump stats"));

// This is running the simplest optimization passes
llvm::cl::opt<std::string> Opt(
    "optimize",
    llvm::cl::desc("Optimization type"),
    llvm::cl::value_desc("optimization name"));

char ProgramPartitionAnalysis::ID = 0;

void ProgramPartitionAnalysis::getAnalysisUsage(llvm::AnalysisUsage& AU) const
{
    AU.addRequired<pdg::SVFGPDGBuilder>();
    AU.addRequired<llvm::LoopInfoWrapperPass>();
    AU.addRequired<llvm::CallGraphWrapperPass>();
    AU.setPreservesAll();
}

bool ProgramPartitionAnalysis::runOnModule(llvm::Module& M)
{
    Logger logger("program-partitioning");
    logger.setLevel(vazgen::Logger::INFO);

    AnnotationParser* annotationParser;
    if (!JsonAnnotations.empty()) {
        annotationParser = new JsonAnnotationParser(&M, JsonAnnotations, logger);
    } else {
        annotationParser = new ModuleAnnotationParser(&M, logger);
    }
    annotationParser->parseAnnotations();
    const auto& annotations = annotationParser->getAllAnnotations();

    auto pdg = getAnalysis<pdg::SVFGPDGBuilder>().getPDG();
    llvm::CallGraph& CG = getAnalysis<llvm::CallGraphWrapperPass>().getCallGraph();
    m_partition.reset(new ProgramPartition(M, pdg, CG, logger));
    const auto& loopGetter = [this] (llvm::Function* F)
        {   return &this->getAnalysis<llvm::LoopInfoWrapperPass>(*F).getLoopInfo(); };
    m_partition->setLoopInfoGetter(loopGetter);
    m_partition->partition(annotations);
    if (!Opt.empty()) {
        const auto& optimizations = getOptimizations(Opt, logger);
        m_partition->optimize(optimizations);
    }
    m_partition->dump(Outfile);
    if (Stats) {
        m_partition->dumpStats();
    }

    return false;
}

static llvm::RegisterPass<ProgramPartitionAnalysis> X("partition-analysis","Runs program partitioning analysis");

} // namespace vazgen

