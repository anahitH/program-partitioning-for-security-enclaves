#pragma once

#include "Optimization/PartitionOptimization.h"

namespace vazgen {

class GlobalsMoveOutPartitionOptimization : public PartitionOptimization
{
public:
    GlobalsMoveOutPartitionOptimization(Partition& partition, PDGType pdg);

    GlobalsMoveOutPartitionOptimization(const GlobalsMoveOutPartitionOptimization& ) = delete;
    GlobalsMoveOutPartitionOptimization(GlobalsMoveOutPartitionOptimization&& ) = delete;
    GlobalsMoveOutPartitionOptimization& operator= (const GlobalsMoveOutPartitionOptimization& ) = delete;
    GlobalsMoveOutPartitionOptimization& operator= (GlobalsMoveOutPartitionOptimization&& ) = delete;

public:
    void run() override;

}; // class GlobalsMoveOutPartitionOptimization

} // namespace vazgen

