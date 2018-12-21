#include "Optimization/DuplicateFunctionsOptimization.h"

#include "PDG/PDG/PDG.h"
#include "PDG/PDG/PDGNode.h"
#include "PDG/PDG/PDGEdge.h"
#include "PDG/PDG/FunctionPDG.h"

namespace vazgen {

DuplicateFunctionsOptimization::DuplicateFunctionsOptimization(Partition& partition)
    : PartitionOptimization(partition, nullptr)
{
}

void DuplicateFunctionsOptimization::
setMovedInFunctions(const Partition::FunctionSet& partitionMovedInFunctions)
{
    m_movedInFs = partitionMovedInFunctions;
}

void DuplicateFunctionsOptimization::
setMovedOutFunctions(const Partition::FunctionSet& partitionMovedOutFunctions)
{
    m_movedOutFs = partitionMovedOutFunctions;
}

void DuplicateFunctionsOptimization::run()
{
    // TODO: duplicate function if it exists in both sets and does not have out arguments and is not modifying any global.
    // OR? can it have out args and modify globals?
}

} // namespace vazgen

