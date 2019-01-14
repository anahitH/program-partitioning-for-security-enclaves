#pragma once

#include "Analysis/BasicBlockLivenessInfo.h"
#include "llvm/Pass.h"

#include <unordered_map>
#include <unordered_set>

namespace llvm {
class BasicBlock;
class LoopInfo;
}

namespace vazgen {

/**
 * \class LivenessAnalysis
 * \brief llvm analysis pass computing live variables for all blocks in the module.
 * Can be ran with opt -live command line argument.
 * Analysis results for each basic block can be requested from this class.
 */
class LivenessAnalysis : public llvm::FunctionPass
{
public:
    using LivenessAnalysisInfo = std::unordered_map<llvm::BasicBlock*, BasicBlockLivenessInfo>;
public:
    static char ID;

    LivenessAnalysis()
        : llvm::FunctionPass(ID)
    {
    }

public:
    void getAnalysisUsage(llvm::AnalysisUsage& AU) const override;
    bool runOnFunction(llvm::Function& F) override;

public:
    const LivenessAnalysisInfo& getLivenessInfo() const;
    LivenessAnalysisInfo& getLivenessInfo();

    const BasicBlockLivenessInfo& getLivenessInfo(llvm::BasicBlock* block) const;
    BasicBlockLivenessInfo& getLivenessInfo(llvm::BasicBlock* block);

private:
    void dag_dfs(llvm::BasicBlock* B);
    void looptree_dfs(llvm::BasicBlock* B);
    bool is_loop_edge(llvm::BasicBlock* S, llvm::BasicBlock* E) const;

    void dump(llvm::Function& F);

private:
    llvm::LoopInfo* m_LI;
    LivenessAnalysisInfo m_blockLivenessInfo;
    std::unordered_set<llvm::BasicBlock*> m_unprocessedBlocks;
}; // class LivenessAnalysis

} // namespace vazgen

