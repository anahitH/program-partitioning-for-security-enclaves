#pragma once

#include <unordered_set>

namespace llvm {
class Function;
class BasicBlock;
class GlobalVariable;
}

namespace vazgen {

class BlockPartition
{
public:
    using FunctionSet = std::unordered_set<llvm::Function*>;
    using BlockSet = std::unordered_set<llvm::BasicBlock*>;
    using GlobalsSet = std::unordered_set<llvm::GlobalVariable*>;

public:
    Partition() = default;

public:
    void setPartition(const BlockSet& blocks);
    void setInInterface(const FunctionSet& functions);
    void setOutInterface(const FunctionSet& functions);
    void setGlobals(const GlobalsSet& globals);

    void setPartition(BlockSet&& blocks);
    void setInInterface(FunctionSet&& functions);
    void setOutInterface(FunctionSet&& functions);
    void setGlobals(GlobalsSet&& globals);

    void addToPartition(llvm::BasicBlock* B);
    void addToPartition(const FunctionSet& functions);
    void addToPartition(const Partition& partition);

    const BlockSet& getPartition() const;
    const FunctionSet& getInInterface() const;
    const FunctionSet& getOutInterface() const;
    const GlobalsSet& getGlobals() const;

    bool contains(llvm::BasicBlock* B) const;
    bool references(llvm::GlobalVariable* global) const;

private:
    BlockSet m_partition;
    FunctionSet m_inInterface;
    FunctionSet m_outInterface;
    GlobalsSet m_partitionGlobals;
}; // class Partition

} // namespace vazgen

