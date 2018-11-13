#include "Transforms/ProgramSlicer.h"

#include "Analysis/Partitioner.h"
#include "Analysis/ProgramPartitionAnalysis.h"
#include "Logger.h"
#include "Statistics.h"

#include "llvm/IR/Function.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Constants.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/raw_ostream.h"

#include "llvm/IR/LegacyPassManager.h"
#include "llvm/PassRegistry.h"
#include "llvm/Transforms/IPO/PassManagerBuilder.h"

#include <algorithm>
#include <iterator>

namespace vazgen {

ProgramSlicer::ProgramSlicer(llvm::Module* M, Slice slice)
    : m_module(M)
    , m_slice(slice)
    , m_slicedModule(nullptr)
{
}

bool ProgramSlicer::slice()
{
    return false;
}

char ProgramSlicerPass::ID = 0;

void ProgramSlicerPass::getAnalysisUsage(llvm::AnalysisUsage& AU) const
{
    AU.addRequired<ProgramPartitionAnalysis>();
    AU.setPreservesAll();
}

bool ProgramSlicerPass::runOnModule(llvm::Module& M)
{
    Logger logger("program-partitioning");
    logger.setLevel(vazgen::Logger::ERR);

    auto partition = getAnalysis<ProgramPartitionAnalysis>().getProgramPartition().getPartition();
    ProgramSlicer::Slice slice(partition.size());
    std::copy(partition.begin(), partition.end(), std::back_inserter(slice));
    m_slicer.reset(new ProgramSlicer(&M, slice));
    bool modified = m_slicer->slice();
    if (modified) {
        logger.info("Slice done\n");
    }
    return modified;
}

static llvm::RegisterPass<ProgramSlicerPass> X("slice-partition","Slice program for a partitioning");

} // namespace vazgen
