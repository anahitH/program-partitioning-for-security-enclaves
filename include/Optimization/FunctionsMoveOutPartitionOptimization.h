#pragma once

#include "Optimization/PartitionOptimization.h"

namespace vazgen {

class FunctionsMoveOutPartitionOptimization : public PartitionOptimization
{
public:
    FunctionsMoveOutPartitionOptimization(Partition& partition, PDGType pdg);

    FunctionsMoveOutPartitionOptimization(const FunctionsMoveOutPartitionOptimization& ) = delete;
    FunctionsMoveOutPartitionOptimization(FunctionsMoveOutPartitionOptimization&& ) = delete;
    FunctionsMoveOutPartitionOptimization& operator= (const FunctionsMoveOutPartitionOptimization& ) = delete;
    FunctionsMoveOutPartitionOptimization& operator= (FunctionsMoveOutPartitionOptimization&& ) = delete;

public:
    void run() override;

}; // class FunctionsMoveOutPartitionOptimization

} // namespace vazgen

