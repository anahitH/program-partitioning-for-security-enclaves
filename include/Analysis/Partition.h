#pragma once

#include <unordered_set>
#include <unordered_map>

namespace llvm {
class Function;
class GlobalVariable;
class BasicBlock;
}

namespace vazgen {

class Partition
{
public:
    using FunctionSet = std::unordered_set<llvm::Function*>;
    using GlobalsSet = std::unordered_set<llvm::GlobalVariable*>;

    // TODO: here temporary
    using BlockSet = std::unordered_set<llvm::BasicBlock*>;
    using FunctionBlocks = std::unordered_map<llvm::Function*, BlockSet>;

public:
    Partition() = default;

public:
    void setPartition(const FunctionSet& functions);
    void setInInterface(const FunctionSet& functions);
    void setOutInterface(const FunctionSet& functions);
    void setGlobals(const GlobalsSet& globals);
    void setSecureBlocks(const FunctionBlocks& functionBlocks);

    void setPartition(FunctionSet&& functions);
    void setInInterface(FunctionSet&& functions);
    void setOutInterface(FunctionSet&& functions);
    void setGlobals(GlobalsSet&& globals);
    void setSecureBlocks(FunctionBlocks&& functionBlocks);

    void addToPartition(llvm::Function* F);
    void addToPartition(const FunctionSet& functions);
    void addToPartition(const Partition& partition);
    void addToPartition(const FunctionBlocks& secureBlocks);

    void removeFromPartition(llvm::Function* F);

    const FunctionSet& getPartition() const;
    const FunctionSet& getInInterface() const;
    const FunctionSet& getOutInterface() const;
    const GlobalsSet& getGlobals() const;
    const FunctionBlocks& getSecureBlocks() const;

    bool contains(llvm::Function* F) const;
    bool references(llvm::GlobalVariable* global) const;

private:
    FunctionSet m_partition;
    FunctionSet m_inInterface;
    FunctionSet m_outInterface;
    GlobalsSet m_partitionGlobals;

    FunctionBlocks m_secureBlocks;
}; // class Partition

} // namespace vazgen

