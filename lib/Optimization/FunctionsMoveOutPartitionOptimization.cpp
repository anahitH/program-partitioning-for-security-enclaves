#include "Optimization/FunctionsMoveOutPartitionOptimization.h"

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

FunctionsMoveOutPartitionOptimization::
FunctionsMoveOutPartitionOptimization(Partition& partition,
                                      PDGType pdg,
                                      const LoopInfoGetter& loopInfoGetter)
    : PartitionOptimization(partition, pdg)
    , m_loopInfoGetter(loopInfoGetter)
{
}

void FunctionsMoveOutPartitionOptimization::run()
{
    for (auto* F : m_partition.getOutInterface()) {
        if (m_partition.contains(F)) {
            continue;
        }
       if (m_movedFunctions.find(F) != m_movedFunctions.end()) {
            continue;
        }
        const auto Fpdg = m_pdg->getFunctionPDG(F);
        const auto& callSites = Fpdg->getCallSites();
        if (hasCallSiteInLoop(callSites)) {
            m_movedFunctions.insert(F);
        }
    }
}

bool FunctionsMoveOutPartitionOptimization::
hasCallSiteInLoop(const CallSites& callSites) const
{
    for (const auto& callSite : callSites) {
        llvm::Function* caller = callSite.getCaller();
        if (m_partition.contains(caller)) {
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

