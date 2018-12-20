#include "Optimization/PartitionOptimization.h"

#include "PDG/PDG/PDG.h"

namespace vazgen {

PartitionOptimization::PartitionOptimization(Partition& partition, PDGType pdg)
    : m_partition(partition)
    , m_pdg(pdg)
{
}

} // namespace vazgen

