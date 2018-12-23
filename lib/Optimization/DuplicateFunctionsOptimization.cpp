#include "Optimization/DuplicateFunctionsOptimization.h"
#include "Utils/Logger.h"

#include "PDG/PDG/PDG.h"
#include "PDG/PDG/PDGNode.h"
#include "PDG/PDG/PDGEdge.h"
#include "PDG/PDG/FunctionPDG.h"

namespace vazgen {

DuplicateFunctionsOptimization::DuplicateFunctionsOptimization(Partition& partition, Logger& logger)
    : PartitionOptimization(partition, nullptr, logger, PartitionOptimizer::DUPLICATE_FUNCTIONS)
{
}

void DuplicateFunctionsOptimization::
setPartitionsFunctions(const Partition::FunctionSet& partition1Fs,
                       const Partition::FunctionSet& partition2Fs)
{
    m_securePartFs = partition1Fs;
    m_insecurePartFs = partition2Fs;
}

void DuplicateFunctionsOptimization::run()
{
    m_logger.info("Running DuplicateFunctions optimization");
    for (auto F : m_securePartFs) {
        if (m_insecurePartFs.find(F) != m_insecurePartFs.end()) {
            m_duplicatedFunctions.insert(F);
        }
    }
}

} // namespace vazgen

