#include "Optimization/PartitionOptimizer.h"

#include "Optimization/FunctionsMoveToPartitionOptimization.h"
#include "Optimization/GlobalsMoveToPartitionOptimization.h"
#include "Optimization/DuplicateFunctionsOptimization.h"
#include "Utils/PartitionUtils.h"
#include "Utils/Logger.h"

#include "PDG/PDG/PDG.h"

#include "llvm/Analysis/LoopInfo.h"

namespace vazgen {

PartitionOptimizer::PartitionOptimizer(Partition& securePartition,
                                       Partition& insecurePartition,
                                       Logger& logger)
    : m_securePartition(securePartition)
    , m_insecurePartition(insecurePartition)
    , m_logger(logger)
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
    m_optimizations.push_back(getOptimizerFor(FUNCTIONS_MOVE_TO, m_securePartition, m_insecurePartition));
    m_optimizations.push_back(getOptimizerFor(FUNCTIONS_MOVE_TO, m_insecurePartition, m_securePartition));
    m_optimizations.push_back(getOptimizerFor(GLOBALS_MOVE_TO, m_securePartition, m_insecurePartition));
    m_optimizations.push_back(getOptimizerFor(GLOBALS_MOVE_TO, m_insecurePartition, m_securePartition));
    m_optimizations.push_back(getOptimizerFor(DUPLICATE_FUNCTIONS, m_securePartition, m_insecurePartition));
}

PartitionOptimizer::OptimizationTy
PartitionOptimizer::getOptimizerFor(PartitionOptimizer::Optimization opt,
                                    Partition& partition,
                                    const Partition& complementPart)
{
    switch (opt) {
    case PartitionOptimizer::FUNCTIONS_MOVE_TO:
        return std::make_shared<FunctionsMoveToPartitionOptimization>(partition, m_pdg, m_logger, m_loopInfoGetter);
    case PartitionOptimizer::GLOBALS_MOVE_TO:
        return std::make_shared<GlobalsMoveToPartitionOptimization>(partition, complementPart.getReferencedGolbals(), m_pdg, m_logger);
    case PartitionOptimizer::DUPLICATE_FUNCTIONS:
        return std::make_shared<DuplicateFunctionsOptimization>(partition, m_logger);
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
    m_logger.info("Applying optimizations");
    for (int i = 0; i < OPT_NUM; ++i) {
        m_optimizations[i]->apply();
    }
    m_securePartition.setInInterface(PartitionUtils::computeInInterface(m_securePartition.getPartition(), *m_pdg));
    m_securePartition.setOutInterface(PartitionUtils::computeOutInterface(m_securePartition.getPartition(), *m_pdg));
    m_insecurePartition.setInInterface(PartitionUtils::computeInInterface(m_insecurePartition.getPartition(), *m_pdg));
    m_insecurePartition.setOutInterface(PartitionUtils::computeOutInterface(m_insecurePartition.getPartition(), *m_pdg));
}

} // namespace vazgen

