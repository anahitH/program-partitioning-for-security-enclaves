#pragma once

#include <unordered_set>

namespace pdg {
class PDG;
} // namespace pdg

namespace llvm {
class Function;
}

namespace vazgen {

class PartitionUtils
{
public:
    using FunctionSet = std::unordered_set<llvm::Function*>;
    static  FunctionSet computeInInterface(const FunctionSet& functions,
                                           const pdg::PDG& pdg);
    static  FunctionSet computeOutInterface(const FunctionSet& functions,
                                            const pdg::PDG& pdg);
}; // class PartitionUtils

} // namespace vazgen

