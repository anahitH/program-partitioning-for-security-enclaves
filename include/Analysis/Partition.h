#pragma once

#include <unordered_set>

namespace llvm {
class Function;
}

namespace vazgen {

class Partition
{
public:
    using FunctionSet = std::unordered_set<llvm::Function*>;

public:
    Partition() = default;
    Partition(const FunctionSet& partition,
              const FunctionSet& inInterface,
              const FunctionSet& outInterface);

    Partition(FunctionSet&& partition,
              FunctionSet&& inInterface,
              FunctionSet&& outInterface);

public:
    void setPartition(const FunctionSet& functions);
    void setInInterface(const FunctionSet& functions);
    void setOutInterface(const FunctionSet& functions);

    void setPartition(FunctionSet&& functions);
    void setInInterface(FunctionSet&& functions);
    void setOutInterface(FunctionSet&& functions);

    void addToPartition(llvm::Function* F);
    void addToPartition(const FunctionSet& functions);
    void addToPartition(const Partition& partition);

    const FunctionSet& getPartition() const;
    const FunctionSet& getInInterface() const;
    const FunctionSet& getOutInterface() const;

private:
    FunctionSet m_partition;
    FunctionSet m_inInterface;
    FunctionSet m_outInterface;
}; // class Partition

} // namespace vazgen

