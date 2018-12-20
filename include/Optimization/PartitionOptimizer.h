#pragma once

#include "Analysis/Partition.h"

#include <memory>
#include <vector>

namespace vazgen {

class PartitionOptimizer
{
public:
    enum Optimization {
        GLOBALS_MOVE_IN,
        GLOBALS_MOVE_OUT,
        FUNCTIONS_MOVE_IN,
        FUNCTIONS_MOVE_OUT
    };

    using OptimizerTy = std::shared_ptr<PartitionOptimizer>;

public:
    explicit PartitionOptimizer(Partition& partition);

    PartitionOptimizer(const PartitionOptimizer& ) = delete;
    PartitionOptimizer(PartitionOptimizer&& ) = delete;
    PartitionOptimizer& operator= (const PartitionOptimizer& ) = delete;
    PartitionOptimizer& operator= (PartitionOptimizer&& ) = delete;

    virtual ~PartitionOptimizer() = default;

public:
    virtual void run();

private:
    void collectAvailableOptimizations();

protected:
    Partition& m_partition;
    std::vector<OptimizerTy> m_optimizations;
}; // class PartitionOptimizer

} // namespace vazgen

