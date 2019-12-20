#include "Optimization/CallbacksOptimization.h"

#include "Analysis/CallGraph.h"
#include "Analysis/Partition.h"
#include "Utils/PartitionUtils.h"
#include "Utils/Logger.h"
// need to use llvm::dbgs()
#include "llvm/Support/Debug.h"
#include "llvm/Support/raw_ostream.h"

#include "PDG/PDG/PDG.h"

#include "llvm/IR/Function.h"

namespace vazgen {

namespace {
std::vector<llvm::Function*> getSourcesForAllInEdgesWithNonCallUse(Node* sinkNode)
{
    std::vector<llvm::Function*> allSources;
    for (auto in_edge_it = sinkNode->inEdgesBegin(); in_edge_it != sinkNode->inEdgesEnd(); ++in_edge_it) {
        const auto& edge = *in_edge_it;
        if (edge.getWeight().hasFactor(WeightFactor::NONCALL_USE)) {
            allSources.push_back(edge.getSource()->getFunction());
        }
    }

    return allSources;
}
}

CallbacksOptimization::CallbacksOptimization(const CallGraph& callgraph,
                                             Partition& securePartition,
                                             Partition& insecurePartition,
                                             Logger& logger)
    : PartitionOptimization(securePartition, nullptr, logger, PartitionOptimizer::ADJUST_CALLBACKS)
    , m_securePartition(securePartition)
    , m_insecurePartition(insecurePartition)
    , m_callGraph(callgraph)
{
}

void CallbacksOptimization::run()
{
    for (auto f_it = m_callGraph.begin(); f_it != m_callGraph.end(); ++f_it) {
        llvm::Function* F1 = f_it->first;
        //llvm::dbgs() << F1->getName() << "\n";
        Partition& F1_partition = m_securePartition.contains(F1) ? m_securePartition : m_insecurePartition;
        auto inEdgeSources = getSourcesForAllInEdgesWithNonCallUse(f_it->second.get());
        if (m_securePartition.contains(F1)) {
            moveFunctionsToSecurePartition(inEdgeSources);
        } else if (std::any_of(inEdgeSources.begin(), inEdgeSources.end(), [this] (llvm::Function* F) { return this->m_securePartition.contains(F);})) {
            inEdgeSources.push_back(F1);
            moveFunctionsToSecurePartition(inEdgeSources);
        }
    }
}

void CallbacksOptimization::moveFunctionsToSecurePartition(const std::vector<llvm::Function*>& functions)
{
    for (const auto& F2 : functions) {
        if (m_securePartition.contains(F2)) {
            continue;
        }
        // add F2 to secure partition
        m_securePartition.addToPartition(F2);
        // remove F2 from F2_partition in_interface
        m_insecurePartition.removeFromInInterface(F2);
        // remove F2 from F1_partition out_interface
        m_securePartition.removeFromOutInterface(F2);
    }
}

} // namespace vazgen

