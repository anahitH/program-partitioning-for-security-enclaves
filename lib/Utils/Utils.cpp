#include "Utils/Utils.h"

#include "PDG/PDG/PDGNode.h"
#include "PDG/PDG/PDGLLVMNode.h"

#include "llvm/IR/Function.h"
#include "llvm/Bitcode/BitcodeWriter.h"
#include "llvm/IR/Module.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Support/raw_ostream.h"

namespace vazgen {

void Utils::saveModule(llvm::Module* M, const std::string& name)
{
    std::error_code EC;
    llvm::raw_fd_ostream OS(name, EC, llvm::sys::fs::OpenFlags::F_None);
    llvm::WriteBitcodeToFile(*M, OS);
    OS.flush();
}

llvm::Function* Utils::getNodeParent(pdg::PDGNode* node)
{
    assert(!llvm::isa<pdg::PDGLLVMFormalArgumentNode>(node));
    auto* llvmNode = llvm::dyn_cast<pdg::PDGLLVMNode>(node);
    if (!llvmNode) {
        return nullptr;
    }
    if (llvm::isa<pdg::PDGLLVMInstructionNode>(llvmNode)) {
        auto* instr = llvm::dyn_cast<llvm::Instruction>(llvmNode->getNodeValue());
        assert(instr);
        return instr->getFunction();
    }
    if (auto* argNode = llvm::dyn_cast<pdg::PDGLLVMFormalArgumentNode>(llvmNode)) {
        return argNode->getFunction();
    }
    if (auto* argNode = llvm::dyn_cast<pdg::PDGLLVMVaArgNode>(llvmNode)) {
        return argNode->getFunction();
    }
    if (auto* argNode = llvm::dyn_cast<pdg::PDGLLVMActualArgumentNode>(llvmNode)) {
        return argNode->getCallSite().getCaller();
    }
    if (auto* blockNode = llvm::dyn_cast<pdg::PDGLLVMBasicBlockNode>(llvmNode)) {
        return blockNode->getBlock()->getParent();
    }
    if (auto* functionNode = llvm::dyn_cast<pdg::PDGLLVMFunctionNode>(llvmNode)) {
        return functionNode->getFunction();
    }
    if (auto* phiNode = llvm::dyn_cast<pdg::PDGPhiNode>(llvmNode)) {
        return phiNode->getBlock(0)->getParent();
    }
    return nullptr;
}

int Utils::getFunctionSize(llvm::Function* F)
{
    int size = 0;
    for (auto& B : *F) {
        size += B.size();
    }
    return size;
}

} // namespace vazgen

