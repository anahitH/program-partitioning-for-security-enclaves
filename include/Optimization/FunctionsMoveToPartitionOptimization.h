#pragma once

#include "Optimization/PartitionOptimization.h"

#include <vector>
#include <functional>

namespace llvm {
class CallSite;
class LoopInfo;
}

namespace vazgen {

class FunctionsMoveToPartitionOptimization : public PartitionOptimization
{
public:
    using CallSites = std::vector<llvm::CallSite>;
    using LoopInfoGetter = std::function<llvm::LoopInfo* (llvm::Function*)>;

public:
    FunctionsMoveToPartitionOptimization(Partition& partition,
                                         PDGType pdg,
                                         const LoopInfoGetter& loopInfoGetter);

    FunctionsMoveToPartitionOptimization(const FunctionsMoveToPartitionOptimization& ) = delete;
    FunctionsMoveToPartitionOptimization(FunctionsMoveToPartitionOptimization&& ) = delete;
    FunctionsMoveToPartitionOptimization& operator= (const FunctionsMoveToPartitionOptimization& ) = delete;
    FunctionsMoveToPartitionOptimization& operator= (FunctionsMoveToPartitionOptimization&& ) = delete;

public:
    void run() override;

    const Partition::FunctionSet& getMovedFunctions() const
    {
        return m_movedFunctions;
    }

    static bool classof(const PartitionOptimization* opt)
    {
        return opt->getOptimizationType() == PartitionOptimizer::FUNCTIONS_MOVE_TO;
    }

private:
    Partition::FunctionSet computeFunctionsCalledFromPartitionOnly();
    Partition::FunctionSet computeFunctionsCalledFromPartitionLoops();
    bool hasCallSiteOutsidePartition(const CallSites& callSites) const;
    bool hasCallSiteInLoop(const CallSites& callSites) const;

private:
    const LoopInfoGetter& m_loopInfoGetter;
    Partition::FunctionSet m_movedFunctions;
}; // class FunctionsMoveToPartitionOptimization

} // namespace vazgen

