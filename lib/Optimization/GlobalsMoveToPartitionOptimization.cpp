#include "Optimization/GlobalsMoveToPartitionOptimization.h"

#include "Utils/Utils.h"

#include "PDG/PDG/PDG.h"
#include "PDG/PDG/PDGEdge.h"
#include "PDG/PDG/PDGNode.h"
#include "PDG/PDG/PDGLLVMNode.h"

namespace vazgen {

GlobalsMoveToPartitionOptimization::
GlobalsMoveToPartitionOptimization(Partition& partition, PDGType pdg)
    : PartitionOptimization(partition, pdg)
{
}

void GlobalsMoveToPartitionOptimization::run()
{
    for (auto* global : m_partition.getReferencedGolbals()) {
        if (!hasUseOutsidePartition(global)) {
            m_movedGlobals.insert(global);
        }
    }
}

bool GlobalsMoveToPartitionOptimization::hasUseOutsidePartition(llvm::GlobalVariable* global) const
{
    auto globalNode = m_pdg->getGlobalVariableNode(global);
    assert(globalNode);
    for (auto in_it = globalNode->inEdgesBegin();
         in_it != globalNode->inEdgesEnd();
         ++in_it) {
        auto sourceNode = (*in_it)->getSource();
        llvm::Function* parent = Utils::getNodeParent(sourceNode.get());
        if (!m_partition.contains(parent)) {
            return true;
        }
    }
    for (auto out_it = globalNode->outEdgesBegin();
         out_it != globalNode->outEdgesEnd();
         ++out_it) {
        auto destNode = (*out_it)->getDestination();
        llvm::Function* parent = Utils::getNodeParent(destNode.get());
        if (!m_partition.contains(parent)) {
            return true;
        }
    }
    return false;
}

} // namespace vazgen

