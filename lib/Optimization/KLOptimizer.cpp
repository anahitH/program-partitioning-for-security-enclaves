#include "Optimization/KLOptimizer.h"

#include "Analysis/CallGraph.h"
#include "Analysis/Partition.h"
#include "Optimization/KLOptimizationPass.h"
#include "Utils/PartitionUtils.h"
#include "Utils/Logger.h"

#include "PDG/PDG/PDG.h"

#include "llvm/IR/Function.h"

namespace vazgen {

class KLOptimizer::Impl
{
public:
    Impl(const CallGraph& callgraph,
         const pdg::PDG& pdg,
         Partition& securePartition,
         Partition& insecurePartition,
         Logger& logger);

public:
    void run();

private:
    KLOptimizationPass::Functions collectPassCandidates() const;

private:
    const CallGraph& m_callGraph;
    const pdg::PDG& m_pdg;
    Partition& m_securePartition;
    Partition& m_insecurePartition;
    Logger& m_logger;
}; // class KLOptimizer::Impl

KLOptimizer::Impl::Impl(const CallGraph& callgraph,
                        const pdg::PDG& pdg,
                        Partition& securePartition,
                        Partition& insecurePartition,
                        Logger& logger)
    : m_callGraph(callgraph)
    , m_pdg(pdg)
    , m_securePartition(securePartition)
    , m_insecurePartition(insecurePartition)
    , m_logger(logger)
{
}

void KLOptimizer::Impl::run()
{
    const auto& passCandidates = collectPassCandidates();
    KLOptimizationPass klOptPass(m_callGraph, m_securePartition, m_insecurePartition, m_logger);

    // One pass should be enough
    //while (true) {
    klOptPass.setCandidates(passCandidates);
    klOptPass.run();
    //}

    m_securePartition.setInInterface(PartitionUtils::computeInInterface(m_securePartition.getPartition(), m_pdg));
    m_securePartition.setOutInterface(PartitionUtils::computeOutInterface(m_securePartition.getPartition(), m_pdg));
    m_insecurePartition.setInInterface(PartitionUtils::computeInInterface(m_insecurePartition.getPartition(), m_pdg));
    m_insecurePartition.setOutInterface(PartitionUtils::computeOutInterface(m_insecurePartition.getPartition(), m_pdg));
}

KLOptimizationPass::Functions KLOptimizer::Impl::collectPassCandidates() const
{
    KLOptimizationPass::Functions passCandidates;
    for (auto F : m_insecurePartition.getPartition()) {
        if (!F->isDeclaration() && !F->isIntrinsic()) {
            passCandidates.push_back(F);
        }
    }
    return passCandidates;
}

KLOptimizer::KLOptimizer(const CallGraph& callgraph,
                         const pdg::PDG& pdg,
                         Partition& securePartition,
                         Partition& insecurePartition,
                         Logger& logger)
    : m_impl(new Impl(callgraph, pdg, securePartition, insecurePartition, logger))
{
}

void KLOptimizer::run()
{
    m_impl->run();
}

} // namespace vazgen

