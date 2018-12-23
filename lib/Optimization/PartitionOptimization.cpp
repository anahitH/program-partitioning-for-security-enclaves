#include "Optimization/PartitionOptimization.h"

#include "Utils/Logger.h"

#include "PDG/PDG/PDG.h"

namespace vazgen {

PartitionOptimization::PartitionOptimization(Partition& partition,
                                             PDGType pdg,
                                             Logger& logger,
                                             PartitionOptimizer::Optimization optimizationType)
    : m_partition(partition)
    , m_pdg(pdg)
    , m_logger(logger)
    , m_optimizationType(optimizationType)
{
}

} // namespace vazgen

