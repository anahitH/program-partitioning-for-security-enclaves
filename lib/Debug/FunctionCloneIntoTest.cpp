#include "Utils/Logger.h"

#include "llvm/IR/Instructions.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/ValueMap.h"
#include "llvm/Pass.h"
#include "llvm/PassRegistry.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Transforms/IPO/PassManagerBuilder.h"
#include "llvm/Transforms/Utils/Cloning.h"
#include "llvm/Transforms/Utils/ValueMapper.h"

#include "llvm/Bitcode/BitcodeWriter.h"

#include "PDG/Passes/PDGBuildPasses.h"
#include "PDG/PDG/PDG.h"
#include "PDG/PDG/PDGNode.h"
#include "PDG/PDG/PDGEdge.h"
#include "PDG/PDG/FunctionPDG.h"

#include <memory>
#include <vector>

namespace debug {

class FunctionCloneIntoTest : public llvm::ModulePass
{
public:
    static char ID;
    FunctionCloneIntoTest()
        : llvm::ModulePass(ID)
    {
    }

    void getAnalysisUsage(llvm::AnalysisUsage& AU) const
    {
        AU.addRequired<pdg::SVFGPDGBuilder>();
    }

    bool runOnModule(llvm::Module& M) override
    {
        m_module = &M;
        m_pdg = getAnalysis<pdg::SVFGPDGBuilder>().getPDG();
        llvm::Function* handlerF = createCallbackHandler();
        for (auto& F : M) {
            if (F.isDeclaration()) {
                continue;
            }
            const auto& callbackArgs = getCallbackArguments(&F);
            if (!callbackArgs.empty()) {
                cloneInto(&F, handlerF);
            }
        }
    }

private:
    llvm::Module* m_module;
    std::shared_ptr<pdg::PDG> m_pdg;

private:
    bool isCallbackArgument(llvm::Type* arg_type)
    {
        if (arg_type->isFunctionTy()) {
            return true;
        }
        if (auto* ptr_ty = llvm::dyn_cast<llvm::PointerType>(arg_type)) {
            if (ptr_ty->getElementType()->isFunctionTy()) {
                return true;
            }
        }
        return false;

    }

    llvm::Function* createCallbackHandler()
    {
        llvm::LLVMContext& Ctx = m_module->getContext();
        llvm::ArrayRef<llvm::Type*> params{llvm::Type::getInt128Ty(Ctx)};
        llvm::FunctionType* fType = llvm::FunctionType::get(llvm::Type::getVoidTy(Ctx),
                params, false);
        std::string fName = "callbackHandler";
        llvm::FunctionCallee functionCallee = m_module->getOrInsertFunction(fName, fType);
        llvm::Function* callbackHandler = llvm::dyn_cast<llvm::Function>(functionCallee.getCallee());
        return callbackHandler;
    }

    std::vector<llvm::Argument*> getCallbackArguments(llvm::Function* F)
    {
        std::vector<llvm::Argument*> callbackArguments;
        for (auto arg_it = F->arg_begin(); arg_it != F->arg_end(); ++arg_it) {
            auto* arg_type = arg_it->getType();
            llvm::dbgs() << *arg_type << "\n";
            if (isCallbackArgument(arg_type)) {
                callbackArguments.push_back(&*arg_it);
            }
        }
        return callbackArguments;
    }

    void cloneInto(llvm::Function* F, llvm::Function* callbackHandler)
    {
        std::vector<llvm::Type*> callbackArguments(F->getFunctionType()->getNumParams());
        int i = 0;
        std::vector<int> callbackArgIdx;
        for (auto arg_it = F->arg_begin(); arg_it != F->arg_end(); ++arg_it) {
            auto* arg_type = arg_it->getType();
            if (isCallbackArgument(arg_type)) {
                callbackArgIdx.push_back(i);
                callbackArguments[i++] = llvm::Type::getInt128Ty(m_module->getContext());
            } else {
                callbackArguments[i++] = arg_it->getType();
            }
        }
        llvm::ArrayRef<llvm::Type*> callbackArgumentsArray(callbackArguments);
        llvm::FunctionType* modifiedFunctionType = llvm::FunctionType::get(F->getReturnType(), callbackArgumentsArray, F->isVarArg());
        // use new_<name> copy and adjust everything there. then remove the old one and rename the new one back
        llvm::Value* calleeValue = m_module->getOrInsertFunction("new_F", modifiedFunctionType).getCallee();
        llvm::dbgs() << *calleeValue << "\n";
        llvm::Function* modifiedF = llvm::dyn_cast<llvm::Function>(calleeValue);
        llvm::ValueToValueMapTy value_to_value_map;
        std::unordered_map<llvm::Instruction*, llvm::Argument*> callbackInvokations;
        // map callback args to their address args
        auto Fpdg = m_pdg->getFunctionPDG(F);
        for (const auto& idx : callbackArgIdx) {
            llvm::Argument* callbackArg = F->getArg(idx);
            llvm::dbgs() << *callbackArg << "\n";
            value_to_value_map[callbackArg] = modifiedF->getArg(idx);
            if (!Fpdg->hasFormalArgNode(F->getArg(idx))) {
                continue;
            }
            auto formalArgNode = Fpdg->getFormalArgNode(F->getArg(idx));
            std::unordered_set<llvm::Value*> processedNodes;
            std::list<pdg::FunctionPDG::PDGNodeTy> workingList;
            workingList.push_back(formalArgNode);
            while (!workingList.empty()) {
                auto currentNode = workingList.back().get();
                workingList.pop_back();
                auto* llvmNode = llvm::dyn_cast<pdg::PDGLLVMNode>(currentNode);
                if (!llvmNode) {
                    continue;
                }
                auto* nodeValue = llvmNode->getNodeValue();
                if (!nodeValue) {
                    continue;
                }
                if (llvm::isa<pdg::PDGLLVMInstructionNode>(llvmNode)) {
                    if (!processedNodes.insert(nodeValue).second) {
                        continue;
                    }
                }
                if (llvm::isa<llvm::CallInst>(nodeValue) || llvm::isa<llvm::InvokeInst>(nodeValue)) {
                    llvm::dbgs() << *nodeValue << "\n";
                    callbackInvokations[llvm::dyn_cast<llvm::Instruction>(nodeValue)] = callbackArg;
                }
            }
        }

        llvm::SmallVector<llvm::ReturnInst*, 2> returns;
        llvm::CloneFunctionInto(modifiedF, F, value_to_value_map, false, returns);

        for (const auto& [callInst, arg] : callbackInvokations) {
            llvm::dbgs() << *callInst << "\n";
            llvm::IRBuilder<> builder(modifiedF->getContext());
            llvm::CallBase* callBase = llvm::dyn_cast<llvm::CallBase>(&*value_to_value_map[callInst]);
            builder.SetInsertPoint(callBase);
            llvm::LoadInst* load = builder.CreateLoad(arg);
            callBase->setCalledFunction(modifiedF);
            const auto& arguments = callBase->args();
            callBase->setArgOperand(0, load);
            int i = 1;
            for (auto arg_it = arguments.begin(); arg_it != arguments.end(); ++arg_it) {
                callBase->setArgOperand(i++, llvm::dyn_cast<llvm::Value>(&*arg_it));
            }
        }
    }
};

char FunctionCloneIntoTest::ID = 0;
static llvm::RegisterPass<FunctionCloneIntoTest> X("f-clone","Clones functions with callback args");


}

