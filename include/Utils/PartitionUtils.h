#pragma once

#include <unordered_set>

namespace pdg {
class PDG;
} // namespace pdg

namespace llvm {
class Function;
class GlobalVariable;
}

namespace vazgen {

class PartitionUtils
{
public:
    using FunctionSet = std::unordered_set<llvm::Function*>;
    using GlobalsSet = std::unordered_set<llvm::GlobalVariable*>;
    static  FunctionSet computeInInterface(const FunctionSet& functions,
                                           const pdg::PDG& pdg);
    static  FunctionSet computeOutInterface(const FunctionSet& functions,
                                            const pdg::PDG& pdg);
    static GlobalsSet computeGlobalsUsedInFunctions(const FunctionSet& functions);
}; // class PartitionUtils

} // namespace vazgen

