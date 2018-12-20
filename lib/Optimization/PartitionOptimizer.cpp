#include "Optimization/PartitionOptimizer.h"

namespace vazgen {

PartitionOptimizer::OptimizerTy getOptimizerFor(PartitionOptimizer::Optimization opt)
{
    switch (opt) {
    case PartitionOptimizer::GLOBALS_MOVE_IN:
    case PartitionOptimizer::GLOBALS_MOVE_OUT:
    case PartitionOptimizer::FUNCTIONS_MOVE_IN:
    case PartitionOptimizer::FUNCTIONS_MOVE_OUT:
    default:
        break;
    }
    return PartitionOptimizer::OptimizerTy();
}

PartitionOptimizer::PartitionOptimizer(Partition& partition)
    : m_partition(partition)
{
    collectAvailableOptimizations();
}

void PartitionOptimizer::run()
{
    for (auto optimizer : m_optimizations) {
        optimizer->run();
    }
}

void PartitionOptimizer::collectAvailableOptimizations()
{
    m_optimizations.push_back(getOptimizerFor(GLOBALS_MOVE_IN));
    m_optimizations.push_back(getOptimizerFor(GLOBALS_MOVE_OUT));
    m_optimizations.push_back(getOptimizerFor(FUNCTIONS_MOVE_IN));
    m_optimizations.push_back(getOptimizerFor(FUNCTIONS_MOVE_OUT));
}

} // namespace vazgen

