#pragma once

#include "Optimization/PartitionOptimization.h"

#include <vector>
#include <functional>

namespace llvm {
class CallSite;
class LoopInfo;
}

namespace vazgen {

class FunctionsMoveOutPartitionOptimization : public PartitionOptimization
{
public:
    using CallSites = std::vector<llvm::CallSite>;
    using LoopInfoGetter = std::function<llvm::LoopInfo* (llvm::Function*)>;

public:
    FunctionsMoveOutPartitionOptimization(Partition& partition,
                                          PDGType pdg,
                                          const LoopInfoGetter& loopInfoGetter);

    FunctionsMoveOutPartitionOptimization(const FunctionsMoveOutPartitionOptimization& ) = delete;
    FunctionsMoveOutPartitionOptimization(FunctionsMoveOutPartitionOptimization&& ) = delete;
    FunctionsMoveOutPartitionOptimization& operator= (const FunctionsMoveOutPartitionOptimization& ) = delete;
    FunctionsMoveOutPartitionOptimization& operator= (FunctionsMoveOutPartitionOptimization&& ) = delete;

public:
    void run() override;

private:
    bool hasCallSiteInLoop(const CallSites& callSites) const;

private:
    const LoopInfoGetter& m_loopInfoGetter;
    Partition::FunctionSet m_movedFunctions;
}; // class FunctionsMoveOutPartitionOptimization

} // namespace vazgen

