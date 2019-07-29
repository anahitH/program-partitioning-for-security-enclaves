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
    addRelatedFunctions(partition);
}

void Partition::addRelatedFunction(llvm::Function* F, int level)
{
    m_relatedFunctions.insert(std::make_pair(F, level));
}

void Partition::addToInInterface(llvm::Function* F)
{
    m_inInterface.insert(F);
}

void Partition::addToOutInterface(llvm::Function* F)
{
    m_outInterface.insert(F);
}

void Partition::removeFromPartition(llvm::Function* F)
{
    m_partition.erase(F);
}

void Partition::removeRelatedFunction(llvm::Function* F)
{
    m_relatedFunctions.erase(F);
}

void Partition::clearRelatedFunctions()
{
    m_relatedFunctions.clear();
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

const std::unordered_map<llvm::Function*, int> Partition::getRelatedFunctions() const
{
    return m_relatedFunctions;
}

int Partition::getFunctionRelationLevel(llvm::Function* F) const
{
    auto pos = m_relatedFunctions.find(F);
    if (pos == m_relatedFunctions.end()) {
        return -1;
    }
    return pos->second;
}

bool Partition::contains(llvm::Function* F) const
{
    return m_partition.find(F) != m_partition.end();
}

bool Partition::containsFunctionWithName(const std::string& Fname) const
{
    for (const auto& F : m_partition) {
        if (F->getName() == Fname) {
            return true;
        }
    }
    return false;
}

bool Partition::contains(llvm::GlobalVariable* g) const
{
    return m_partitionGlobals.find(g) != m_partitionGlobals.end();
}

bool Partition::references(llvm::GlobalVariable* global) const
{
    return m_partitionGlobals.find(global) != m_partitionGlobals.end();
}

void Partition::addRelatedFunctions(const Partition& partition)
{
    const auto& relatedFunctions = partition.getRelatedFunctions();
    for (const auto& [function, level] : relatedFunctions) {
        auto [pair, inserted] = m_relatedFunctions.insert(std::make_pair(function, level));
        if (!inserted) {
            pair->second = std::min(pair->second, level);
        }
    }
}

} // namespace vazgen

