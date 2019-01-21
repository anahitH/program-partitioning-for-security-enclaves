#include "Optimization/PartitionOptimizer.h"

#include "Optimization/FunctionsMoveToPartitionOptimization.h"
#include "Optimization/GlobalsMoveToPartitionOptimization.h"
#include "Optimization/DuplicateFunctionsOptimization.h"
#include "Optimization/KLOptimizer.h"
#include "Optimization/StaticAnalysisOptimization.h"
#include "Optimization/ILPOptimization.h"
#include "Utils/PartitionUtils.h"
#include "Utils/Logger.h"

#include "PDG/PDG/PDG.h"

#include "llvm/Analysis/LoopInfo.h"

namespace vazgen {

PartitionOptimizer::PartitionOptimizer(Partition& securePartition,
                                       Partition& insecurePartition,
                                       PDGType pdg,
                                       const CallGraph& callgraph,
                                       Logger& logger)
    : m_securePartition(securePartition)
    , m_insecurePartition(insecurePartition)
    , m_pdg(pdg)
    , m_callgraph(callgraph)
    , m_logger(logger)
{
}

void PartitionOptimizer::setLoopInfoGetter(const LoopInfoGetter& loopInfoGetter)
{
    m_loopInfoGetter = loopInfoGetter;
}

void PartitionOptimizer::run(const Optimizations& opts)
{
    for (auto opt : opts) {
        m_optimizations.push_back(getOptimizerFor(opt, m_securePartition, m_insecurePartition));
        if (opt == DUPLICATE_FUNCTIONS) {
            runDuplicateFunctionsOptimization(m_optimizations.back());
            continue;
        }
        m_optimizations.back()->run();
        if (opt == FUNCTIONS_MOVE_TO || opt == GLOBALS_MOVE_TO) {
            m_optimizations.push_back(getOptimizerFor(opt, m_insecurePartition, m_securePartition));
            m_optimizations.back()->run();
        }
    }
    apply();
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
        return std::make_shared<GlobalsMoveToPartitionOptimization>(partition, complementPart.getGlobals(), m_pdg, m_logger);
    case PartitionOptimizer::DUPLICATE_FUNCTIONS:
        return std::make_shared<DuplicateFunctionsOptimization>(partition, m_logger);
    case KERNIGHAN_LIN:
        return std::make_shared<KLOptimizer>(m_callgraph, m_pdg, m_securePartition, m_insecurePartition, m_logger);
    case STATIC_ANALYSIS:
        return std::make_shared<StaticAnalysisOptimization>(m_securePartition, m_logger);
    case ILP:
        return std::make_shared<ILPOptimization>(m_callgraph, m_securePartition, m_logger);
    default:
        break;
    }
    assert(false);
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
    for (auto opt : m_optimizations) {
        opt->apply();
    }
    m_securePartition.setInInterface(PartitionUtils::computeInInterface(m_securePartition.getPartition(), *m_pdg));
    m_securePartition.setOutInterface(PartitionUtils::computeOutInterface(m_securePartition.getPartition(), *m_pdg));
    m_insecurePartition.setInInterface(PartitionUtils::computeInInterface(m_insecurePartition.getPartition(), *m_pdg));
    m_insecurePartition.setOutInterface(PartitionUtils::computeOutInterface(m_insecurePartition.getPartition(), *m_pdg));
}

} // namespace vazgen

