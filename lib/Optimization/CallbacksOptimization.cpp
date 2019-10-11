#include "Optimization/CallbacksOptimization.h"

#include "Analysis/CallGraph.h"
#include "Analysis/Partition.h"
#include "Utils/PartitionUtils.h"
#include "Utils/Logger.h"

#include "PDG/PDG/PDG.h"

#include "llvm/IR/Function.h"

namespace vazgen {

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
        Partition& F1_partition = m_securePartition.contains(F1) ? m_securePartition : m_insecurePartition;
        // iterate over the edges 
        for (auto out_edge_it = f_it->second->outEdgesBegin(); out_edge_it != f_it->second->outEdgesEnd(); ++out_edge_it) {
            const auto& edge = *out_edge_it;
            llvm::Function* F2 = edge.getSink()->getFunction();
            if (edge.getWeight().hasFactor(WeightFactor::NONCALL_USE)) {
                // TODO: refactor
                if ((m_securePartition.contains(F1) && m_securePartition.contains(F2))
                        || (m_insecurePartition.contains(F1) && m_insecurePartition.contains(F2))) {
                    continue;
                }
                Partition& F2_partition = m_securePartition.contains(F2) ? m_securePartition : m_insecurePartition;
                // add F2 to F1_partition
                F1_partition.addToPartition(F2);
                // remove F2 from F2_partition in_interface
                F2_partition.removeFromInInterface(F2);
                // remove F2 from F1_partition out_interface
                F1_partition.removeFromOutInterface(F2);
            }
        }
    }
}

} // namespace vazgen

