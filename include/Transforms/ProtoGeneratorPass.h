#pragma once

#include "llvm/Pass.h"

namespace vazgen {

class ProtoGeneratorPass : public llvm::ModulePass
{
public:
    static char ID;

    ProtoGeneratorPass()
        : llvm::ModulePass(ID)
    {
    }

public:
    void getAnalysisUsage(llvm::AnalysisUsage& AU) const override;
    bool runOnModule(llvm::Module& M) override;
}; // class ProtoGeneratorPass

} // namespace vazgen


