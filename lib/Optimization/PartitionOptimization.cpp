#include "Optimization/PartitionOptimization.h"

#include "PDG/PDG/PDG.h"

namespace vazgen {

PartitionOptimization::PartitionOptimization(Partition& partition,
                                             PDGType pdg,
                                             PartitionOptimizer::Optimization optimizationType)
    : m_partition(partition)
    , m_pdg(pdg)
    , m_optimizationType(optimizationType)
{
}

} // namespace vazgen

