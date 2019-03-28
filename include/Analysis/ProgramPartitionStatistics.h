#pragma once

#include "llvm/Pass.h"

namespace vazgen {

class ProgramPartitionStatisticsPass : public llvm::ModulePass
{
public:
    static char ID;

    ProgramPartitionStatisticsPass()
        : llvm::ModulePass(ID)
    {
    }

public:
    void getAnalysisUsage(llvm::AnalysisUsage& AU) const override;
    bool runOnModule(llvm::Module& M) override;
}; // class ProgramPartitionStatisticsPass

} // namespace vazgen

