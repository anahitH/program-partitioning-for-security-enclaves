#include "Utils/Logger.h"

#include "llvm/IR/Instructions.h"
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
        const std::string function_name = F->getName();
        const std::string clone_name = function_name + "_clone";
        auto* F_decl = llvm::dyn_cast<llvm::Function>(M.getOrInsertFunction(clone_name, functionType));

        bool modified = false;
        for (auto it = F->user_begin(); it != F->user_end(); ++it) {
            modified |= change_use(*it, F, F_decl);
        }
        if (!modified) {
            F_decl->eraseFromParent();
            return false;
        }
        llvm::ValueToValueMapTy value_to_value_map;
        auto new_M = llvm::CloneModule(&M, value_to_value_map,
                [&function_name] (const llvm::GlobalValue* glob) {
                    return glob->getName() == function_name;
                }
                );
        new_M->setModuleIdentifier("lib");
        for (auto it = new_M->begin(); it != new_M->end(); ++it) {
            if (it->isDeclaration() && it->user_empty()) {
                auto* new_F = &*it;
                ++it;
                new_F->dropAllReferences();
                new_F->eraseFromParent();
            }
        }
        F->dropAllReferences();
        F->eraseFromParent();
        F_decl->setName(function_name);

        std::error_code EC;
        llvm::raw_fd_ostream OS("lib.bc", EC, llvm::sys::fs::OpenFlags::F_None);
        llvm::WriteBitcodeToFile(new_M.get(), OS);
        OS.flush();

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
        return true;
    }
}; // class FunctionExtractorPass

char FunctionExtractorPass::ID = 0;
static llvm::RegisterPass<FunctionExtractorPass> X("extract","Extract given function");

}

