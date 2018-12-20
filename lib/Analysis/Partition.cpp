#include "Analysis/Partition.h"

#include "llvm/IR/Function.h"
#include "llvm/IR/GlobalVariable.h"

namespace vazgen {

void Partition::setPartition(const FunctionSet& functions)
{
    m_partition = functions;
}

void Partition::setInInterface(const FunctionSet& functions)
{
    m_inInterface = functions;
}

void Partition::setOutInterface(const FunctionSet& functions)
{
    m_outInterface = functions;
}

void Partition::setReferencedGlobals(const GlobalsSet& globals)
{
    m_referencedGlobals = globals;
}

void Partition::setModifiedGlobals(const GlobalsSet& globals)
{
    m_modifiedGlobals = globals;
}

void Partition::setPartition(FunctionSet&& functions)
{
    m_partition = std::move(functions);
}

void Partition::setInInterface(FunctionSet&& functions)
{
    m_inInterface = std::move(functions);
}

void Partition::setOutInterface(FunctionSet&& functions)
{
    m_outInterface = std::move(functions);
}

void Partition::setReferencedGlobals(GlobalsSet&& globals)
{
    m_referencedGlobals = std::move(globals);
}

void Partition::setModifiedGlobals(GlobalsSet&& globals)
{
    m_modifiedGlobals = std::move(globals);
}

void Partition::addToPartition(llvm::Function* F)
{
    m_partition.insert(F);
}

void Partition::addToPartition(const FunctionSet& functions)
{
    m_partition.insert(functions.begin(), functions.end());
}

void Partition::addToPartition(const Partition& partition)
{
    m_partition.insert(partition.m_partition.begin(), partition.m_partition.end());
    m_inInterface.insert(partition.m_inInterface.begin(), partition.m_inInterface.end());
    m_outInterface.insert(partition.m_outInterface.begin(), partition.m_outInterface.end());
    m_referencedGlobals.insert(partition.m_referencedGlobals.begin(), partition.m_referencedGlobals.end());
    m_referencedGlobals.insert(partition.m_modifiedGlobals.begin(), partition.m_modifiedGlobals.end());
}

void Partition::addReferencedGlobal(llvm::GlobalVariable* global)
{
    m_referencedGlobals.insert(global);
}

void Partition::addModifiedGlobal(llvm::GlobalVariable* global)
{
    m_modifiedGlobals.insert(global);
}

void Partition::addReferencedGlobals(const GlobalsSet& globals)
{
    m_referencedGlobals.insert(globals.begin(), globals.end());
}

void Partition::addModifiedGlobals(const GlobalsSet& globals)
{
    m_modifiedGlobals.insert(globals.begin(), globals.end());
}

const Partition::FunctionSet& Partition::getPartition() const
{
    return m_partition;
}

const Partition::FunctionSet& Partition::getInInterface() const
{
    return m_inInterface;
}

const Partition::FunctionSet& Partition::getOutInterface() const
{
    return m_outInterface;
}

const Partition::GlobalsSet& Partition::getReferencedGolbals() const
{
    return m_referencedGlobals;
}

const Partition::GlobalsSet& Partition::getModifiedGlobals() const
{
    return m_modifiedGlobals;
}

} // namespace vazgen

