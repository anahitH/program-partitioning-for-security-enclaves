#include "Logger.h"

#include "llvm/Pass.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Instructions.h"
#include "llvm/PassRegistry.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Transforms/IPO/PassManagerBuilder.h"

namespace debug {

llvm::cl::opt<std::string> function(
    "function",
    llvm::cl::desc("Function name to extract"),
    llvm::cl::value_desc("function name"));

class FunctionExtractorPass : public llvm::ModulePass
{
public:
    static char ID;
    FunctionExtractorPass()
        : llvm::ModulePass(ID)
    {
    }

    bool runOnModule(llvm::Module& M) override
    {
        vazgen::Logger logger("test");
        logger.setLevel(vazgen::Logger::ERR);

        if (function.empty()) {
            logger.error("No function is given for extraction\n");
            return false;
        }
        if (function == "main") {
            logger.error("Can not extract main\n");
            return false;
        }
        auto* F = M.getFunction(function);
        if (!F) {
            logger.error("No function with the given name\n");
            return false;
        }
        if (F->isDeclaration()) {
            logger.error("Function's definition is not in a module. Nothing to extract.\n");
            return false;
        }
        auto* functionType = F->getFunctionType();
        const std::string clone_name = F->getName().str() + "_clone";
        auto* F_decl = llvm::dyn_cast<llvm::Function>(M.getOrInsertFunction(clone_name, functionType));

        bool modified = false;
        for (auto it = F->user_begin(); it != F->user_end(); ++it) {
            modified |= change_use(*it, F, F_decl);
        }
        if (modified) {
            F->dropAllReferences();
            F->eraseFromParent();
        } else {
            F_decl->eraseFromParent();
        }

        return modified;
    }

    bool change_use(llvm::User* user, llvm::Function* orig_F, llvm::Function* clone_F)
    {
        if (auto* callInst = llvm::dyn_cast<llvm::CallInst>(user)) {
            callInst->setCalledFunction(clone_F);
        } else if (auto* invokeInst = llvm::dyn_cast<llvm::InvokeInst>(user)) {
            invokeInst->setCalledFunction(clone_F);
        } else {
            user->replaceUsesOfWith(orig_F, clone_F);
        }
    }

}; // class FunctionExtractorPass

char FunctionExtractorPass::ID = 0;
static llvm::RegisterPass<FunctionExtractorPass> X("extract","Extract given function");

}

