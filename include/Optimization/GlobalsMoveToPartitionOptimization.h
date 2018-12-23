#pragma once

#include "Optimization/PartitionOptimization.h"

namespace vazgen {

class GlobalsMoveToPartitionOptimization : public PartitionOptimization
{
public:
    GlobalsMoveToPartitionOptimization(Partition& partition, PDGType pdg);

    GlobalsMoveToPartitionOptimization(const GlobalsMoveToPartitionOptimization& ) = delete;
    GlobalsMoveToPartitionOptimization(GlobalsMoveToPartitionOptimization&& ) = delete;
    GlobalsMoveToPartitionOptimization& operator= (const GlobalsMoveToPartitionOptimization& ) = delete;
    GlobalsMoveToPartitionOptimization& operator= (GlobalsMoveToPartitionOptimization&& ) = delete;

public:
    void run() override;

private:
    bool hasUseOutsidePartition(llvm::GlobalVariable* global) const;

    static bool classof(const PartitionOptimization* opt)
    {
        return opt->getOptimizationType() == PartitionOptimizer::GLOBALS_MOVE_TO;
    }

private:
    Partition::GlobalsSet m_movedGlobals;
}; // class GlobalsMoveToPartitionOptimization

} // namespace vazgen

