#include "Analysis/ProgramPartitionAnalysis.h"

#include "Analysis/Partitioner.h"
#include "Analysis/PartitionStatistics.h"
#include "Utils/Logger.h"
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
    if (optName == "static-analysis") {
        opts.push_back(PartitionOptimizer::STATIC_ANALYSIS);
    } else if (optName == "local") {
        opts.push_back(PartitionOptimizer::FUNCTIONS_MOVE_TO);
        opts.push_back(PartitionOptimizer::GLOBALS_MOVE_TO);
        //opts.push_back(PartitionOptimizer::DUPLICATE_FUNCTIONS);
    } else if (optName == "function-move") {
        opts.push_back(PartitionOptimizer::FUNCTIONS_MOVE_TO);
    } else if (optName == "global-move") {
        opts.push_back(PartitionOptimizer::GLOBALS_MOVE_TO);
    } else if (optName == "function-duplicate") {
        opts.push_back(PartitionOptimizer::DUPLICATE_FUNCTIONS);
    } else if (optName == "kl") {
        opts.push_back(PartitionOptimizer::KERNIGHAN_LIN);
    } else if (optName == "ilp") {
        opts.push_back(PartitionOptimizer::ILP);
    } else {
        logger.error("No optimization with name " + optName);
    }
    return opts;
}

}

ProgramPartition::ProgramPartition(llvm::Module& M,
                                   PDGType pdg,
                                   const llvm::CallGraph& callgraph,
                                   const LoopInfoGetter& loopInfoGetter,
                                   Logger& logger)
    : m_module(M)
    , m_pdg(pdg)
    , m_callgraph(callgraph)
    , m_loopInfoGetter(loopInfoGetter)
    , m_logger(logger)
{
}

void ProgramPartition::partition(const Annotations& annotations)
{
    Partitioner partitioner(m_module, m_pdg, m_logger);
    partitioner.partition(annotations);
    m_securePartition = partitioner.getSecurePartition();
    m_insecurePartition = partitioner.getInsecurePartition();
    m_callgraph.assignWeights(m_securePartition, m_insecurePartition, m_pdg.get(), m_loopInfoGetter);
}

void ProgramPartition::optimize(auto optimizations)
{
    PartitionOptimizer optimizer(m_securePartition, m_insecurePartition, m_pdg, m_callgraph, m_logger);
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
    PartitionStatistics stats(strm, m_securePartition, m_callgraph, m_module);
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
    const auto& loopGetter = [this] (llvm::Function* F)
        {   return &this->getAnalysis<llvm::LoopInfoWrapperPass>(*F).getLoopInfo(); };
    m_partition.reset(new ProgramPartition(M, pdg, CG, loopGetter, logger));
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

