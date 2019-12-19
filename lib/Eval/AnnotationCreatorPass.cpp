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

#include "nlohmann/json.hpp"
#include <algorithm>
#include <ctime>
#include <fstream>
#include <iomanip>
#include <iterator>
#include <cstdlib>
#include <vector>
#include <random>
#include <algorithm>
#include <iterator>

namespace vazgen {

class AnnotationCreatorPass : public llvm::ModulePass
{
public:
    static char ID;

    AnnotationCreatorPass()
        : llvm::ModulePass(ID)
    {
    }

public:
    bool runOnModule(llvm::Module& M) override;
};


llvm::cl::opt<int> Percent(
    "percent",
    llvm::cl::desc("Percentage of functions to annotate"),
    llvm::cl::value_desc("percentage"));

llvm::cl::opt<std::string> AnnotFile(
    "annotation-file",
    llvm::cl::desc("Out file to write annotations"),
    llvm::cl::value_desc("file name"));


using Functions = std::vector<llvm::Function*>;
Functions pickFunctions(llvm::Module& M, int percentage)
{
    Functions functionsToAnnotate;
    if (percentage == 0) {
        return functionsToAnnotate;
    }
    Functions allFunctions;
    allFunctions.reserve(M.size());
    for (auto& F : M) {
        if (!F.isDeclaration() && !F.isIntrinsic() && F.getName() != "main") {
            allFunctions.push_back(&F);
        }
    }
    std::random_device rd;
    std::mt19937 g(rd());
    std::shuffle(allFunctions.begin(), allFunctions.end(), g);

    int num = allFunctions.size() * percentage/100;
    functionsToAnnotate.reserve(num);

    int numOfAllFunctions = allFunctions.size();
    srand(time(NULL));
    for (int i = 0; i < num; ++i) {
        int idx = rand() % (numOfAllFunctions);
        //llvm::dbgs() << "Randomly chosen index " << idx << "\n";
        //llvm::dbgs() << "Randomly chosen function " << allFunctions[idx]->getName() << "\n";
        functionsToAnnotate.push_back(allFunctions[idx]);
        //llvm::dbgs() << "idx " << idx << " function: " << allFunctions[idx]->getName() << "\n";
        std::swap(allFunctions[idx], allFunctions[numOfAllFunctions - 1]);
        --numOfAllFunctions;
    }

    return functionsToAnnotate;
}

void dumpAnnotationFile(const Functions& functions, const std::string& annotFileName)
{
    std::ofstream annotStrm;
    annotStrm.open(annotFileName);
    nlohmann::json annot;
    annot["annotation"] = "sensitive";
    auto& functionsElement = annot["functions"];
    for (auto* F : functions) {
        nlohmann::json f_element;
        f_element["function"] = F->getName();
        for (auto arg_it = F->arg_begin(); arg_it != F->arg_end(); ++arg_it) {
            if (arg_it->getType()->isPointerTy()) {
                f_element["arguments"].push_back(arg_it->getArgNo());
            }
        }
        if (!F->getReturnType()->isVoidTy()) {
            f_element["return"] = "True";
        }
        functionsElement.push_back(f_element);
    }
    nlohmann::json annotations;
    annotations.push_back(annot);
    annotStrm << std::setw(4) << annotations;
    annotations.clear();
}

bool AnnotationCreatorPass::runOnModule(llvm::Module& M)
{
    int percentage = Percent;
    std::string annot_file = AnnotFile.empty() ? M.getName().str() + "_annotations.json" : AnnotFile;
    const auto& functions = pickFunctions(M, percentage);
    dumpAnnotationFile(functions, annot_file);
    return false;
}

char AnnotationCreatorPass::ID = 0;
static llvm::RegisterPass<AnnotationCreatorPass> X("gen-annotation","Generate annotations file");

} // namespace vazgen
