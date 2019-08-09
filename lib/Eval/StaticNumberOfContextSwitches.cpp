#include "llvm/Pass.h"

#include "Analysis/CallGraph.h"
#include "Analysis/ProgramPartitionAnalysis.h"

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

#include <fstream>

namespace vazgen{

class StaticNumberOfContextSwitches : public llvm::ModulePass
{
public:
    static char ID;

    StaticNumberOfContextSwitches()
        : llvm::ModulePass(ID)
    {
    }

public:
    void getAnalysisUsage(llvm::AnalysisUsage& AU) const override;
    bool runOnModule(llvm::Module& M) override;
};

void StaticNumberOfContextSwitches::getAnalysisUsage(llvm::AnalysisUsage& AU) const
{
    AU.setPreservesAll();
    AU.setPreservesCFG();
    AU.addRequired<CallGraphPass>();
    AU.addPreserved<ProgramPartitionAnalysis>();
}

bool StaticNumberOfContextSwitches::runOnModule(llvm::Module& M)
{
    const CallGraph& CG = getAnalysis<CallGraphPass>().getCallGraph();
    const auto& secure_partition = &getAnalysisIfAvailable<vazgen::ProgramPartitionAnalysis>()->getProgramPartition().getSecurePartition();

    double numberOfContextSwitches = 0;
    std::unordered_set<llvm::Function*> processed_functions;

    for (auto it = CG.begin(); it != CG.end(); ++it) {
        assert(processed_functions.insert(it->first).second);
        for (auto edge_it = it->second->outEdgesBegin(); edge_it != it->second->outEdgesEnd(); ++edge_it) {
            auto* callee = edge_it->getSink()->getFunction();
            if (!secure_partition->contains(it->first) || !secure_partition->contains(callee)) {
                numberOfContextSwitches += edge_it->getWeight().getFactor(WeightFactor::CALL_NUM).getValue().getValue();
            }
        }
    }
    std::ofstream strm;
    strm.open("number_of_ctx_switches.txt");
    strm << numberOfContextSwitches;
    strm.close();

    return false;
}

char StaticNumberOfContextSwitches::ID = 0;
static llvm::RegisterPass<StaticNumberOfContextSwitches> X("ctx-switches","Computes number of context switches");


}
