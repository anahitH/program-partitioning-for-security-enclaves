#pragma once

#include "Analysis/Partition.h"

#include "Optimization/PartitionOptimizer.h"

#include <memory>
#include <vector>

namespace pdg {
class PDG;
}

namespace vazgen {

class Logger;

class PartitionOptimization
{
public:
    using PDGType = std::shared_ptr<pdg::PDG>;

public:
    PartitionOptimization(Partition& partition,
                          PDGType pdg,
                          Logger& logger,
                          PartitionOptimizer::Optimization optimizationType);

    PartitionOptimization(const PartitionOptimization& ) = delete;
    PartitionOptimization(PartitionOptimization&& ) = delete;
    PartitionOptimization& operator= (const PartitionOptimization& ) = delete;
    PartitionOptimization& operator= (PartitionOptimization&& ) = delete;

    virtual ~PartitionOptimization() = default;

public:
    virtual void run() = 0;

    virtual void apply()
    {
    }

    PartitionOptimizer::Optimization getOptimizationType() const
    {
        return m_optimizationType;
    }

protected:
    Partition& m_partition;
    PDGType m_pdg;
    Logger& m_logger;
    PartitionOptimizer::Optimization m_optimizationType;
}; // class PartitionOptimization

} // namespace vazgen

