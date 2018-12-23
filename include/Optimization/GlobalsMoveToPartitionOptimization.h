#pragma once

#include "Optimization/PartitionOptimization.h"

namespace vazgen {

class Logger;

class GlobalsMoveToPartitionOptimization : public PartitionOptimization
{
public:
    GlobalsMoveToPartitionOptimization(Partition& moveToPartition,
                                       const Partition::GlobalsSet& outsideUses,
                                       PDGType pdg,
                                       Logger& logger);

    GlobalsMoveToPartitionOptimization(const GlobalsMoveToPartitionOptimization& ) = delete;
    GlobalsMoveToPartitionOptimization(GlobalsMoveToPartitionOptimization&& ) = delete;
    GlobalsMoveToPartitionOptimization& operator= (const GlobalsMoveToPartitionOptimization& ) = delete;
    GlobalsMoveToPartitionOptimization& operator= (GlobalsMoveToPartitionOptimization&& ) = delete;

public:
    void run() override;
    void apply() override;

private:
    static bool classof(const PartitionOptimization* opt)
    {
        return opt->getOptimizationType() == PartitionOptimizer::GLOBALS_MOVE_TO;
    }

private:
    const Partition::GlobalsSet& m_globals;
    Partition::GlobalsSet m_movedGlobals;
}; // class GlobalsMoveToPartitionOptimization

} // namespace vazgen

