#pragma once

#include <unordered_set>
#include <unordered_map>

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
    void addRelatedFunction(llvm::Function* F, int level);

    void removeFromPartition(llvm::Function* F);
    void removeRelatedFunction(llvm::Function* F);
    void clearRelatedFunctions();

    const FunctionSet& getPartition() const;
    const FunctionSet& getInInterface() const;
    const FunctionSet& getOutInterface() const;
    const GlobalsSet& getGlobals() const;
    const std::unordered_map<llvm::Function*, int> getRelatedFunctions() const;
    int getFunctionRelationLevel(llvm::Function* F) const;

    void removeFromInInterface(llvm::Function* F);
    void removeFromOutInterface(llvm::Function* F);

    bool contains(llvm::Function* F) const;
    // this is expensive function
    bool containsFunctionWithName(const std::string& Fname) const;
    bool contains(llvm::GlobalVariable* g) const;
    bool references(llvm::GlobalVariable* global) const;

private:
    void addRelatedFunctions(const Partition& partition);

private:
    std::unordered_map<llvm::Function*, int> m_relatedFunctions;
    FunctionSet m_partition;
    FunctionSet m_inInterface;
    FunctionSet m_outInterface;
    GlobalsSet m_partitionGlobals;
}; // class Partition

} // namespace vazgen

