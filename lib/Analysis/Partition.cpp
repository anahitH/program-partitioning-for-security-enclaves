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

void Partition::setGlobals(const GlobalsSet& globals)
{
    m_partitionGlobals = globals;
}

void Partition::setSecureBlocks(const FunctionBlocks& functionBlocks)
{
    m_secureBlocks = functionBlocks;
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

void Partition::setGlobals(GlobalsSet&& globals)
{
    m_partitionGlobals = std::move(globals);
}

void Partition::setSecureBlocks(FunctionBlocks&& functionBlocks)
{
    m_secureBlocks = std::move(functionBlocks);
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
    m_partitionGlobals.insert(partition.m_partitionGlobals.begin(), partition.m_partitionGlobals.end());
    m_secureBlocks.insert(partition.getSecureBlocks().begin(), partition.getSecureBlocks().end());
}

void Partition::addToPartition(const FunctionBlocks& secureBlocks)
{
    m_secureBlocks.insert(secureBlocks.begin(), secureBlocks.end());
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

const Partition::GlobalsSet& Partition::getGlobals() const
{
    return m_partitionGlobals;
}

const Partition::FunctionBlocks& Partition::getSecureBlocks() const
{
    return m_secureBlocks;
}

bool Partition::contains(llvm::Function* F) const
{
    return m_partition.find(F) != m_partition.end();
}

bool Partition::references(llvm::GlobalVariable* global) const
{
    return m_partitionGlobals.find(global) != m_partitionGlobals.end();
}

} // namespace vazgen

