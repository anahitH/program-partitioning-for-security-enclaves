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
    void setReferencedGlobals(const GlobalsSet& globals);
    void setModifiedGlobals(const GlobalsSet& globals);

    void setPartition(FunctionSet&& functions);
    void setInInterface(FunctionSet&& functions);
    void setOutInterface(FunctionSet&& functions);
    void setReferencedGlobals(GlobalsSet&& globals);
    void setModifiedGlobals(GlobalsSet&& globals);

    void addToPartition(llvm::Function* F);
    void addToPartition(const FunctionSet& functions);
    void addToPartition(const Partition& partition);
    void addReferencedGlobal(llvm::GlobalVariable* global);
    void addModifiedGlobal(llvm::GlobalVariable* global);
    void addReferencedGlobals(const GlobalsSet& globals);
    void addModifiedGlobals(const GlobalsSet& globals);

    const FunctionSet& getPartition() const;
    const FunctionSet& getInInterface() const;
    const FunctionSet& getOutInterface() const;
    const GlobalsSet& getReferencedGolbals() const;
    const GlobalsSet& getModifiedGlobals() const;

    bool contains(llvm::Function* F) const;

private:
    FunctionSet m_partition;
    FunctionSet m_inInterface;
    FunctionSet m_outInterface;
    GlobalsSet m_referencedGlobals;
    GlobalsSet m_modifiedGlobals;
}; // class Partition

} // namespace vazgen

