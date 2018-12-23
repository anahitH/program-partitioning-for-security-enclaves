#include "Optimization/PartitionOptimizer.h"

#include "Optimization/FunctionsMoveOutPartitionOptimization.h"
#include "Optimization/FunctionsMoveToPartitionOptimization.h"
#include "Optimization/GlobalsMoveOutPartitionOptimization.h"
#include "Optimization/GlobalsMoveToPartitionOptimization.h"
#include "Optimization/DuplicateFunctionsOptimization.h"

#include "PDG/PDG/PDG.h"

#include "llvm/Analysis/LoopInfo.h"

namespace vazgen {

PartitionOptimizer::PartitionOptimizer(Partition& securePartition,
                                       Partition& insecurePartition)
    : m_securePartition(securePartition)
    , m_insecurePartition(insecurePartition)
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
    for (int i = 0; i < OPT_NUM; ++i) {
        m_optimizations[i]->run();
        if (i == DUPLICATE_FUNCTIONS) {
            runDuplicateFunctionsOptimization(m_optimizations[i]);
        }
    }
    apply();
}

void PartitionOptimizer::collectAvailableOptimizations()
{
    m_optimizations.push_back(getOptimizerFor(FUNCTIONS_MOVE_TO, m_securePartition));
    m_optimizations.push_back(getOptimizerFor(FUNCTIONS_MOVE_TO, m_insecurePartition));
    m_optimizations.push_back(getOptimizerFor(GLOBALS_MOVE_TO, m_securePartition));
    m_optimizations.push_back(getOptimizerFor(GLOBALS_MOVE_TO, m_insecurePartition));
    m_optimizations.push_back(getOptimizerFor(DUPLICATE_FUNCTIONS, m_securePartition));
}

PartitionOptimizer::OptimizationTy
PartitionOptimizer::getOptimizerFor(PartitionOptimizer::Optimization opt, Partition& partition)
{
    switch (opt) {
    case PartitionOptimizer::FUNCTIONS_MOVE_TO:
        return std::make_shared<FunctionsMoveToPartitionOptimization>(partition, m_pdg, m_loopInfoGetter);
    case PartitionOptimizer::GLOBALS_MOVE_TO:
        return std::make_shared<GlobalsMoveToPartitionOptimization>(partition, m_pdg);
    case PartitionOptimizer::DUPLICATE_FUNCTIONS:
        return std::make_shared<DuplicateFunctionsOptimization>(partition);
    default:
        break;
    }
    return PartitionOptimizer::OptimizationTy();
}

void PartitionOptimizer::runDuplicateFunctionsOptimization(OptimizationTy opt)
{
    auto* duplicateFsOpt = llvm::dyn_cast<DuplicateFunctionsOptimization>(opt.get());
    auto* secureFsMoveTo = llvm::dyn_cast<FunctionsMoveToPartitionOptimization>(m_optimizations[FUNCTIONS_MOVE_TO].get());
    auto* insecureFsMoveTo = llvm::dyn_cast<FunctionsMoveToPartitionOptimization>(m_optimizations[FUNCTIONS_MOVE_TO + 1].get());
    assert(duplicateFsOpt);
    assert(secureFsMoveTo);
    assert(insecureFsMoveTo);
    duplicateFsOpt->setPartitionsFunctions(secureFsMoveTo->getMovedFunctions(),
                                           insecureFsMoveTo->getMovedFunctions());
    duplicateFsOpt->run();
}

void PartitionOptimizer::apply()
{
}

} // namespace vazgen

