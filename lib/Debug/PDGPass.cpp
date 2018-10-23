#include "SVF/PDG/PDGPointerAnalysis.h"
#include "SVF/MSSA/SVFG.h"
#include "SVF/MSSA/SVFGBuilder.h"
#include "SVF/Util/SVFModule.h"

#include "llvm/Pass.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/IR/Module.h"
#include "llvm/PassRegistry.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Transforms/IPO/PassManagerBuilder.h"

namespace debug {

class PDGCreator : public llvm::ModulePass
{
public:
    static char ID;
    PDGCreator()
        : llvm::ModulePass(ID)
    {
    }

    bool runOnModule(llvm::Module& M) override
    {
        SVFModule svfModule(M);
        AndersenWaveDiff* ander = new svfg::PDGAndersenWaveDiff();
        ander->disablePrintStat();
        ander->analyze(svfModule);
        SVFGBuilder memSSA(true);
        SVFG *svfg = memSSA.buildSVFG((BVDataPTAImpl*)ander);
        return false;
    }

}; // class PDGCreator

char PDGCreator::ID = 0;
static llvm::RegisterPass<PDGCreator> X("pdg","Create pdg");

} // namespace debug

