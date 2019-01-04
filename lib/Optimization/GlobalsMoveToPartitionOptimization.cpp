#include "Optimization/GlobalsMoveToPartitionOptimization.h"

#include "Utils/Utils.h"
#include "Utils/Logger.h"

#include "PDG/PDG/PDG.h"
#include "PDG/PDG/PDGEdge.h"
#include "PDG/PDG/PDGNode.h"
#include "PDG/PDG/PDGLLVMNode.h"

namespace vazgen {

GlobalsMoveToPartitionOptimization::
GlobalsMoveToPartitionOptimization(Partition& moveToPartition,
                                   const Partition::GlobalsSet& outsideUses,
                                   PDGType pdg,
                                   Logger& logger)
    : PartitionOptimization(moveToPartition, pdg, logger, PartitionOptimizer::GLOBALS_MOVE_TO)
    , m_globals(outsideUses)
{
}

void GlobalsMoveToPartitionOptimization::run()
{
    m_logger.info("Running GlobalsMoveToPartition optimization");
    for (auto* global : m_partition.getGlobals()) {
        if (m_globals.find(global) == m_globals.end()) {
            m_movedGlobals.insert(global);
        }
    }
}

void GlobalsMoveToPartitionOptimization::apply()
{
    m_partition.setGlobals(std::move(m_movedGlobals));
}

} // namespace vazgen

