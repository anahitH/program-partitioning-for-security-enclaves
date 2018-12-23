#include "Optimization/FunctionsMoveToPartitionOptimization.h"

#include "PDG/PDG/PDG.h"
#include "PDG/PDG/PDGNode.h"
#include "PDG/PDG/PDGEdge.h"
#include "PDG/PDG/FunctionPDG.h"

#include "llvm/Analysis/LoopInfo.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/CallSite.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Module.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/raw_ostream.h"

namespace vazgen {

FunctionsMoveToPartitionOptimization::
FunctionsMoveToPartitionOptimization(Partition& partition,
                                     PDGType pdg,
                                     const LoopInfoGetter& loopInfoGetter)
    : PartitionOptimization(partition, pdg, PartitionOptimizer::FUNCTIONS_MOVE_TO)
    , m_loopInfoGetter(loopInfoGetter)
{
}

void FunctionsMoveToPartitionOptimization::run()
{
    // TODO: needs to re-compute partition's in and out interfaces
    auto functions = computeFunctionsCalledFromPartitionOnly();
    m_movedFunctions.insert(functions.begin(), functions.end());
    functions = computeFunctionsCalledFromPartitionLoops();
    m_movedFunctions.insert(functions.begin(), functions.end());
}

Partition::FunctionSet
FunctionsMoveToPartitionOptimization::computeFunctionsCalledFromPartitionOnly()
{
    // Move function to partition if it's called from partition only
    // Operate on out-interface as out-interface contains all calls from enclave to outside world
    Partition::FunctionSet functionsToMove;
    for (auto* F : m_partition.getOutInterface()) {
        if (m_movedFunctions.find(F) != m_movedFunctions.end()) {
            continue;
        }
        const auto Fpdg = m_pdg->getFunctionPDG(F);
        const auto& callSites = Fpdg->getCallSites();
        if (!hasCallSiteOutsidePartition(callSites)) {
            functionsToMove.insert(F);
        }
    }
    return functionsToMove;
}

Partition::FunctionSet
FunctionsMoveToPartitionOptimization::computeFunctionsCalledFromPartitionLoops()
{
    // Move function from out-interface to partition if it's called in loop
    Partition::FunctionSet functionsToMove;
    for (auto* F : m_partition.getOutInterface()) {
        if (m_movedFunctions.find(F) != m_movedFunctions.end()) {
            continue;
        }
        const auto Fpdg = m_pdg->getFunctionPDG(F);
        const auto& callSites = Fpdg->getCallSites();
        if (hasCallSiteInLoop(callSites)) {
            functionsToMove.insert(F);
        }
    }
    return functionsToMove;
}

bool FunctionsMoveToPartitionOptimization::
hasCallSiteOutsidePartition(const CallSites& callSites) const
{
    for (const auto& callSite : callSites) {
        if (!m_partition.contains(callSite.getCaller())) {
            return true;
        }
    }
    return false;
}

bool FunctionsMoveToPartitionOptimization::
hasCallSiteInLoop(const CallSites& callSites) const
{
    for (const auto& callSite : callSites) {
        llvm::Function* caller = callSite.getCaller();
        if (!m_partition.contains(caller)) {
            continue;
        }
        auto* loopInfo = m_loopInfoGetter(caller);
        if (!loopInfo) {
            continue;
        }
        llvm::BasicBlock* parent = callSite.getParent();
        llvm::Loop* loop = loopInfo->getLoopFor(parent);
        if (loop) {
            return true;
        }
    }
    return false;
}

} // namespace vazgen

