#pragma once

#include <unordered_set>

namespace llvm {
class Function;
class GlobalVariable;
}

namespace vazgen {

class Partition
{
public:
    using FunctionSet = std::unordered_set<llvm::Function*>;
    using GlobalsSet = std::unordered_set<llvm::GlobalVariable*>;

public:
    Partition() = default;

public:
    void setPartition(const FunctionSet& functions);
    void setInInterface(const FunctionSet& functions);
    void setOutInterface(const FunctionSet& functions);
    void setGlobals(const GlobalsSet& globals);

    void setPartition(FunctionSet&& functions);
    void setInInterface(FunctionSet&& functions);
    void setOutInterface(FunctionSet&& functions);
    void setGlobals(GlobalsSet&& globals);

    void addToPartition(llvm::Function* F);
    void addToPartition(const FunctionSet& functions);
    void addToPartition(const Partition& partition);

    const FunctionSet& getPartition() const;
    const FunctionSet& getInInterface() const;
    const FunctionSet& getOutInterface() const;
    const GlobalsSet& getGlobals() const;

    bool contains(llvm::Function* F) const;
    bool references(llvm::GlobalVariable* global) const;

private:
    FunctionSet m_partition;
    FunctionSet m_inInterface;
    FunctionSet m_outInterface;
    GlobalsSet m_partitionGlobals;
}; // class Partition

} // namespace vazgen

