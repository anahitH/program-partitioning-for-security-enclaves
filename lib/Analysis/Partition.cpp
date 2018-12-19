#include "Analysis/Partition.h"

#include "llvm/IR/Function.h"

namespace vazgen {

Partition::Partition(const FunctionSet& partition,
                     const FunctionSet& inInterface,
                     const FunctionSet& outInterface)
    : m_partition(partition)
    , m_inInterface(inInterface)
    , m_outInterface(outInterface)
{
}

Partition::Partition(FunctionSet&& partition,
                     FunctionSet&& inInterface,
                     FunctionSet&& outInterface)
    : m_partition(std::move(partition))
    , m_inInterface(std::move(inInterface))
    , m_outInterface(std::move(outInterface))
{
}

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

} // namespace vazgen

