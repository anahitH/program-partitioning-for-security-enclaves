#include "Utils/PartitionUtils.h"

#include "PDG/PDG/PDG.h"
#include "PDG/PDG/PDGNode.h"
#include "PDG/PDG/PDGEdge.h"
#include "PDG/PDG/FunctionPDG.h"

#include "llvm/IR/Function.h"
#include "llvm/IR/CallSite.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/raw_ostream.h"

namespace vazgen {

PartitionUtils::FunctionSet
PartitionUtils::computeInInterface(const FunctionSet& functions,
                                   const pdg::PDG& pdg)
{
    FunctionSet inInterface;
    for (const auto& F : functions) {
        if (!F) {
            continue;
        }
        if (!pdg.hasFunctionPDG(F)) {
            continue;
        }
        assert(pdg.hasFunctionPDG(F));
        const auto& Fpdg = pdg.getFunctionPDG(F);
        const auto& callSites = Fpdg->getCallSites();
        for (const auto& callSite : callSites) {
            auto* caller = callSite.getCaller();
            if (functions.find(caller) == functions.end()) {
                inInterface.insert(F);
                break;
            }
        }
    }
    return inInterface;
}

PartitionUtils::FunctionSet
PartitionUtils::computeOutInterface(const FunctionSet& functions,
                                    const pdg::PDG& pdg)
{
    FunctionSet outInterface;
    for (const auto& F : functions) {
        if (!F) {
            continue;
        }
        if (!pdg.hasFunctionPDG(F)) {
            continue;
        }
        assert(pdg.hasFunctionPDG(F));
        const auto& Fpdg = pdg.getFunctionPDG(F);
        // TODO: think about having call site information embedded in PDG directly.
        for (auto it = Fpdg->llvmNodesBegin(); it != Fpdg->llvmNodesEnd(); ++it) {
            llvm::Value* val = it->first;
            if (!llvm::dyn_cast<llvm::CallInst>(val)
                    && !llvm::dyn_cast<llvm::InvokeInst>(val)) {
                continue;
            }
            for (auto edgeIt = it->second->outEdgesBegin();
                    edgeIt != it->second->outEdgesEnd();
                    ++edgeIt) {
                if (!(*edgeIt)->isControlEdge()) {
                    continue;
                }
                if (auto* functionNode =
                        llvm::dyn_cast<pdg::PDGLLVMFunctionNode>((*edgeIt)->getDestination().get())) {
                    llvm::Function* outF = functionNode->getFunction();
                    if (functions.find(outF) == functions.end()) {
                        outInterface.insert(outF);
                    }
                }
            }
        }
    }
    return outInterface;
}

} // namespace vazgen

