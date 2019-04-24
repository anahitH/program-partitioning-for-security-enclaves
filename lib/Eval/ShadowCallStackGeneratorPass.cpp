#include "llvm/Pass.h"

#include "llvm/IR/Function.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/raw_ostream.h"

#include "llvm/IR/LegacyPassManager.h"
#include "llvm/PassRegistry.h"
#include "llvm/Transforms/IPO/PassManagerBuilder.h"

namespace vazgen{

class ShadowCallStackGeneratorPass : public llvm::ModulePass
{
public:
    static char ID;

    ShadowCallStackGeneratorPass()
        : llvm::ModulePass(ID)
    {
    }

public:
    bool runOnModule(llvm::Module& M) override;

private:
    void addCaller(llvm::Function* F, llvm::Instruction* beforeInst);
    void addCallee(llvm::Function* F, llvm::Function* callee, llvm::Instruction* beforeInst);
    void addCallee(llvm::Function* F);

private:
    llvm::Function* m_addCallerF;
    llvm::Function* m_addCalleeF;
}; // class ShadowCallStackGeneratorPass

bool ShadowCallStackGeneratorPass::runOnModule(llvm::Module& M)
{
    llvm::LLVMContext& Ctx = M.getContext();
    llvm::FunctionType* addCallerCalleeFTy = llvm::FunctionType::get(llvm::Type::getVoidTy(Ctx), {llvm::Type::getInt8PtrTy(Ctx)});
    m_addCallerF = llvm::dyn_cast<llvm::Function>(M.getOrInsertFunction("addCaller", addCallerCalleeFTy));
    m_addCalleeF = llvm::dyn_cast<llvm::Function>(M.getOrInsertFunction("addCallee", addCallerCalleeFTy));

    for (auto& F : M) {
        if (F.isIntrinsic()) {
            continue;
        }
        if (F.isDeclaration()) {
            continue;
        }
        if (F.getName() != "main") {
            addCallee(&F);
        }
        bool isCaller = false;
        for (auto& B : F) {
            for (auto& I : B) {
                llvm::Function* callee = nullptr;
                if (auto* callInst = llvm::dyn_cast<llvm::CallInst>(&I)) {
                    callee = callInst->getCalledFunction();
                } else if (auto* invokeInst = llvm::dyn_cast<llvm::InvokeInst>(&I)) {
                    callee = invokeInst->getCalledFunction();
                }
                if (!callee || callee->isIntrinsic()) {
                    continue;
                }
                if (callee == m_addCalleeF || callee == m_addCallerF) {
                    continue;
                }
                isCaller = (callee != nullptr);
                addCaller(&F, &I);
                if (callee->isDeclaration() && !callee->isIntrinsic()) {
                    addCallee(&F, callee, &I);
                }
            }
        }
    }
    return true;
}

void ShadowCallStackGeneratorPass::addCaller(llvm::Function* F, llvm::Instruction* beforeInst)
{
    llvm::LLVMContext& Ctx = F->getContext();
    llvm::IRBuilder<> builder(Ctx);
    builder.SetInsertPoint(beforeInst);
    llvm::Value *callerNameStr = builder.CreateGlobalStringPtr(F->getName().str().c_str());
    builder.CreateCall(m_addCallerF, callerNameStr);
}

void ShadowCallStackGeneratorPass::addCallee(llvm::Function* F, llvm::Function* callee, llvm::Instruction* beforeInst)
{
    llvm::LLVMContext& Ctx = F->getContext();
    llvm::IRBuilder<> builder(Ctx);
    builder.SetInsertPoint(beforeInst);
    llvm::Value *callerNameStr = builder.CreateGlobalStringPtr(callee->getName().str().c_str());
    builder.CreateCall(m_addCalleeF, callerNameStr);
}

void ShadowCallStackGeneratorPass::addCallee(llvm::Function* F)
{
    llvm::LLVMContext& Ctx = F->getContext();
    llvm::IRBuilder<> builder(Ctx);
    builder.SetInsertPoint(&*F->getEntryBlock().begin());
    llvm::Value *callerNameStr = builder.CreateGlobalStringPtr(F->getName().str().c_str());
    builder.CreateCall(m_addCalleeF, callerNameStr);
}

char ShadowCallStackGeneratorPass::ID = 0;
static llvm::RegisterPass<ShadowCallStackGeneratorPass> X("gen-shadow-stack","Instrument the binary to generate shadow call stack");


} // namespace vazgen

