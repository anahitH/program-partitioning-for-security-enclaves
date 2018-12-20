#include "Optimization/PartitionOptimizer.h"

#include "Optimization/FunctionsMoveOutPartitionOptimization.h"
#include "Optimization/FunctionsMoveToPartitionOptimization.h"
#include "Optimization/GlobalsMoveOutPartitionOptimization.h"
#include "Optimization/GlobalsMoveToPartitionOptimization.h"

#include "PDG/PDG/PDG.h"

#include "llvm/Analysis/LoopInfo.h"

namespace vazgen {

PartitionOptimizer::PartitionOptimizer(Partition& partition)
    : m_partition(partition)
{
    collectAvailableOptimizations();
}

void PartitionOptimizer::setPDG(PDGType pdg)
{
    m_pdg = pdg;
}

void PartitionOptimizer::setLoopInfoGetter(const LoopInfoGetter& loopInfoGetter)
{
    m_loopInfoGetter = loopInfoGetter;
}

void PartitionOptimizer::run()
{
    for (auto optimizer : m_optimizations) {
        optimizer->run();
    }
}

void PartitionOptimizer::collectAvailableOptimizations()
{
    m_optimizations.push_back(getOptimizerFor(FUNCTIONS_MOVE_TO));
    m_optimizations.push_back(getOptimizerFor(FUNCTIONS_MOVE_OUT));
    m_optimizations.push_back(getOptimizerFor(GLOBALS_MOVE_TO));
    m_optimizations.push_back(getOptimizerFor(GLOBALS_MOVE_OUT));
}

PartitionOptimizer::OptimizationTy
PartitionOptimizer::getOptimizerFor(PartitionOptimizer::Optimization opt)
{
    switch (opt) {
    case PartitionOptimizer::FUNCTIONS_MOVE_TO:
        return std::make_shared<FunctionsMoveToPartitionOptimization>(m_partition, m_pdg, m_loopInfoGetter);
    case PartitionOptimizer::FUNCTIONS_MOVE_OUT:
        return std::make_shared<FunctionsMoveOutPartitionOptimization>(m_partition, m_pdg);
    case PartitionOptimizer::GLOBALS_MOVE_TO:
        return std::make_shared<GlobalsMoveToPartitionOptimization>(m_partition, m_pdg);
    case PartitionOptimizer::GLOBALS_MOVE_OUT:
        return std::make_shared<GlobalsMoveOutPartitionOptimization>(m_partition, m_pdg);
    //case PartitionOptimizer::DUPLICATE_FUNCTIONS:
    default:
        break;
    }
    return PartitionOptimizer::OptimizationTy();
}


} // namespace vazgen

