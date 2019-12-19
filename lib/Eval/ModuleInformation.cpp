#include "llvm/Pass.h"

#include "llvm/IR/Function.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/raw_ostream.h"

#include "llvm/IR/LegacyPassManager.h"
#include "llvm/PassRegistry.h"
#include "llvm/Transforms/IPO/PassManagerBuilder.h"

#include "Utils/Utils.h"

namespace vazgen {

class ModuleInformation : public llvm::ModulePass
{
public:
    static char ID;

    ModuleInformation()
        : llvm::ModulePass(ID)
    {
    }

public:
    bool runOnModule(llvm::Module& M) override
    {
        int num_functions = 0;
        int loc = 0;
        for (auto& F : M) {
            ++num_functions;
            if (!F.isDeclaration()) {
                loc += Utils::getFunctionSize(&F);
            }
        }
        llvm::dbgs() << "Number of funcitons: " << num_functions << "\n";
        llvm::dbgs() << "LOC: " << loc << "\n";

        return false;
    }
};

char ModuleInformation::ID = 0;
static llvm::RegisterPass<ModuleInformation> X("module-info","Dumps module information.");

}
