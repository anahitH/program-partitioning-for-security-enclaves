#pragma once

#include "Optimization/PartitionOptimization.h"

namespace vazgen {

class DuplicateFunctionsOptimization : public PartitionOptimization
{
public:
    DuplicateFunctionsOptimization(Partition& partition);

    DuplicateFunctionsOptimization(const DuplicateFunctionsOptimization& ) = delete;
    DuplicateFunctionsOptimization(DuplicateFunctionsOptimization&& ) = delete;
    DuplicateFunctionsOptimization& operator= (const DuplicateFunctionsOptimization& ) = delete;
    DuplicateFunctionsOptimization& operator= (DuplicateFunctionsOptimization&& ) = delete;

public:
    void setMovedInFunctions(const Partition::FunctionSet& partitionMovedInFunctions);
    void setMovedOutFunctions(const Partition::FunctionSet& partitionMovedOutFunctions);

    void run() override;

private:
    Partition::FunctionSet m_movedInFs;
    Partition::FunctionSet m_movedOutFs;
    Partition::FunctionSet m_duplicatedFunctions;
}; // class DuplicateFunctionsOptimization


} // namespace vazgen

