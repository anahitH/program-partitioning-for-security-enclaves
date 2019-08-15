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

    bool runOnModule(llvm::Module& M) override
    {
        m_module = &M;
        llvm::Function* handlerF = createCallbackHandler();
        for (auto& F : M) {
            const auto& callbackArgs = getCallbackArguments(&F);
            if (!callbackArgs.empty()) {
                cloneInto(&F, handlerF);
            }
        }
    }

private:
    llvm::Module* m_module;

private:
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
            if (arg_type->isFunctionTy()) {
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
            if (arg_type->isFunctionTy()) {
                callbackArgIdx.push_back(i);
                callbackArguments[i++] = llvm::Type::getInt128Ty(m_module->getContext());
            } else {
                callbackArguments[i++] = arg_it->getType();
            }
        }
        llvm::ArrayRef<llvm::Type*> callbackArgumentsArray(callbackArguments);
        llvm::FunctionType* modifiedFunctionType = llvm::FunctionType::get(F->getReturnType(), callbackArgumentsArray, F->isVarArg());
        llvm::Function* modifiedF = llvm::dyn_cast<llvm::Function>(
                m_module->getOrInsertFunction(F->getName(), modifiedFunctionType).getCallee());
        llvm::ValueToValueMapTy value_to_value_map;
        std::unordered_map<llvm::Instruction*, llvm::Argument*> callbackInvokations;
        // map callback args to their address args
        for (const auto& idx : callbackArgIdx) {
            llvm::Argument* callbackArg = F->getArg(idx);
            value_to_value_map[callbackArg] = modifiedF->getArg(idx);
            for (auto user_it = callbackArg->user_begin(); user_it != callbackArg->user_end(); ++user_it) {
                if (llvm::isa<llvm::CallInst>(*user_it)
                        || llvm::isa<llvm::InvokeInst>(*user_it)) {
                    callbackInvokations[llvm::dyn_cast<llvm::Instruction>(*user_it)] = callbackArg;
                    //TODO: try to modify in the clone, if doesn't work change in the original function and adjust using value mapper
                    //llvm::CallInst* handlerCall = createCallToCallbackHandlerForArg(F, callbackArg);
                }
            }
        }

        llvm::SmallVector<llvm::ReturnInst*, 2> returns;
        llvm::CloneFunctionInto(modifiedF, F, value_to_value_map, false, returns);

        for (const auto& [callInst, arg] : callbackInvokations) {
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

