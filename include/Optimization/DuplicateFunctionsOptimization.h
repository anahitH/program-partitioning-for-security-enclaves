#pragma once

#include "Optimization/PartitionOptimization.h"

namespace vazgen {

class Logger;

class DuplicateFunctionsOptimization : public PartitionOptimization
{
public:
    DuplicateFunctionsOptimization(Partition& partition, Logger& logger);

    DuplicateFunctionsOptimization(const DuplicateFunctionsOptimization& ) = delete;
    DuplicateFunctionsOptimization(DuplicateFunctionsOptimization&& ) = delete;
    DuplicateFunctionsOptimization& operator= (const DuplicateFunctionsOptimization& ) = delete;
    DuplicateFunctionsOptimization& operator= (DuplicateFunctionsOptimization&& ) = delete;

public:
    void setPartitionsFunctions(const Partition::FunctionSet& partition1Fs,
                                const Partition::FunctionSet& partition2Fs);

    void run() override;

public:
    static bool classof(const PartitionOptimization* opt)
    {
        return opt->getOptimizationType() == PartitionOptimizer::DUPLICATE_FUNCTIONS;
    }

private:
    Partition::FunctionSet m_securePartFs;
    Partition::FunctionSet m_insecurePartFs;
    Partition::FunctionSet m_duplicatedFunctions;
}; // class DuplicateFunctionsOptimization


} // namespace vazgen

