#include "Analysis/ProgramPartitionStatistics.h"

#include "Analysis/CallGraph.h"
#include "Analysis/ProgramPartitionAnalysis.h"
#include "Analysis/PartitionStatistics.h"
#include "Analysis/Partition.h"
#include "Utils/Logger.h"

#include "PDG/Passes/PDGBuildPasses.h"

#include "llvm/Analysis/CallGraph.h"
#include "llvm/Analysis/LoopInfo.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/Module.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/raw_ostream.h"

#include "llvm/IR/LegacyPassManager.h"
#include "llvm/PassRegistry.h"
#include "llvm/Transforms/IPO/PassManagerBuilder.h"

#include <fstream>

namespace vazgen {

llvm::cl::opt<std::string> StatsFile(
    "stats-file",
    llvm::cl::desc("Out file to write partition statistics"),
    llvm::cl::value_desc("statistics file name"));

char ProgramPartitionStatisticsPass::ID = 0;

void ProgramPartitionStatisticsPass::getAnalysisUsage(llvm::AnalysisUsage& AU) const
{
    AU.addRequired<ProgramPartitionAnalysis>();
    AU.addRequired<llvm::CallGraphWrapperPass>();
    AU.addRequired<llvm::LoopInfoWrapperPass>();
    AU.addRequired<pdg::SVFGPDGBuilder>();
    AU.setPreservesAll();
}

bool ProgramPartitionStatisticsPass::runOnModule(llvm::Module& M)
{
    const auto& insecurePartition = getAnalysis<ProgramPartitionAnalysis>().getProgramPartition().getInsecurePartition();
    const auto& securePartition = getAnalysis<ProgramPartitionAnalysis>().getProgramPartition().getSecurePartition();
    llvm::CallGraph& CG = getAnalysis<llvm::CallGraphWrapperPass>().getCallGraph();
    auto pdg = getAnalysis<pdg::SVFGPDGBuilder>().getPDG();
    const auto& loopGetter = [this] (llvm::Function* F)
        {   return &this->getAnalysis<llvm::LoopInfoWrapperPass>(*F).getLoopInfo(); };

    Logger logger("Program partition statistics");
    logger.setLevel(vazgen::Logger::INFO);

    CallGraph callGraph(pdg.get(), logger);
    callGraph.assignWeights(securePartition, insecurePartition, pdg.get(), loopGetter);
    std::ofstream strm;
    if (StatsFile.empty()) {
        strm.open("partition_stats.json");
    } else {
        strm.open(StatsFile);
    }
    PartitionStatistics stats(strm, securePartition, insecurePartition, callGraph, M);
    stats.report();

    return false;
}

static llvm::RegisterPass<ProgramPartitionStatisticsPass> X("partition-stats","Dumps Partition statistics");
} // namespace vazgen

