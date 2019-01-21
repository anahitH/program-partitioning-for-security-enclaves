#pragma once

#include "Optimization/PartitionOptimization.h"

namespace vazgen {

class StaticAnalysisOptimization : public PartitionOptimization
{
public:
    StaticAnalysisOptimization(Partition& partition, Logger& logger)
        : PartitionOptimization(partition, nullptr, logger, PartitionOptimizer::STATIC_ANALYSIS)
    {
    }

    StaticAnalysisOptimization(const StaticAnalysisOptimization& ) = delete;
    StaticAnalysisOptimization(StaticAnalysisOptimization&& ) = delete;
    StaticAnalysisOptimization& operator= (const StaticAnalysisOptimization& ) = delete;
    StaticAnalysisOptimization& operator= (StaticAnalysisOptimization&& ) = delete;

public:
    void run() override
    {
    }

    void apply() override
    {
        for (const auto& [function, level] : m_partition.getRelatedFunctions()) {
            m_partition.addToPartition(function);
        }
        m_partition.clearRelatedFunctions();
    }

public:
    static bool classof(const PartitionOptimization* opt)
    {
        return opt->getOptimizationType() == PartitionOptimizer::STATIC_ANALYSIS;
    }
}; // class StaticAnalysisOptimization

} // namespace vazgen

