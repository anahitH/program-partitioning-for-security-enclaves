#include "Analysis/LivenessAnalysis.h"

#include "llvm/Analysis/LoopInfo.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Instructions.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/raw_ostream.h"

#include "llvm/PassRegistry.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/Transforms/IPO/PassManagerBuilder.h"

namespace vazgen {

namespace {

// Value v is phiUsed in block B, if it is the ith value in a phinode of successor block succ, where index i corresponds to B.
void update_block_phi_uses_from_successor(BasicBlockLivenessInfo& blockInfo, llvm::BasicBlock* succ)
{
    auto firstNonPhi = succ->getFirstNonPHI();
    if (!firstNonPhi) {
        return; // no phi
    }
    auto it = succ->begin();
    while (&*it != firstNonPhi && it != succ->end()) {
        auto* phi = llvm::dyn_cast<llvm::PHINode>(&*it);
        assert(phi);
        auto val = phi->getIncomingValueForBlock(blockInfo.get_block());
        blockInfo.add_phiUse(val);
        ++it;
    }
}

void update_block_phi_defs(llvm::BasicBlock* B, BasicBlockLivenessInfo& blockInfo)
{
    auto firstNonPhi = B->getFirstNonPHI();
    if (!firstNonPhi) {
        return; // no phi
    }
    BasicBlockLivenessInfo::ValueSet defs;
    auto it = B->begin();
    while (&*it != firstNonPhi && it != B->end()) {
        auto* phi = llvm::dyn_cast<llvm::PHINode>(&*it);
        assert(phi);
        defs.insert(phi);
        ++it;
    }
    blockInfo.set_phiDefs(defs);
}

void update_block_liveins(llvm::BasicBlock* B,
                          BasicBlockLivenessInfo::ValueSet& live)
{
    auto it = B->rbegin();
    while (it != B->rend()) {
        llvm::Instruction* I = &*it;
        ++it;
        for (llvm::Use &u : I->operands()) {
            llvm::Value *v = u.get();
            if (llvm::dyn_cast<llvm::Constant>(v)
                || !llvm::dyn_cast<llvm::Instruction>(v)) {
                continue;
            }
            if (llvm::dyn_cast<llvm::AllocaInst>(v)) {
                live.insert(v);
            }
        }
        // TODO: is an instruction other than store, that need to be handled here?
        if (auto* store = llvm::dyn_cast<llvm::StoreInst>(I)) {
            live.erase(store->getPointerOperand());
        }
    }
}

} // unnamed namespace

void LivenessAnalysis::getAnalysisUsage(llvm::AnalysisUsage& AU) const
{
    AU.addRequired<llvm::LoopInfoWrapperPass>();
    AU.setPreservesAll();
}

/*
   Function Compute_LiveSets_SSA_Reducible(CFG)
   for each basic block B do
        unmark B
   Let R be the root node of the CFG
   DAG_DFS(R)
   for each root node L of the loop-nesting forest do
        LoopTree_DFS(L)
*/
bool LivenessAnalysis::runOnFunction(llvm::Function& F)
{
    llvm::dbgs() << "Function Liveness analysis " << F.getName() << "\n";
    m_blockLivenessInfo.clear();
    m_unprocessedBlocks.clear();
    m_LI = &getAnalysis<llvm::LoopInfoWrapperPass>().getLoopInfo();

    for (auto& B : F) {
        m_unprocessedBlocks.insert(&B);
    }
    auto cfg_root = &F.getEntryBlock();
    dag_dfs(cfg_root);

    for (auto& L : *m_LI) {
        looptree_dfs(L->getHeader());
    }

    //dump(F);
    return false;
}

const LivenessAnalysis::LivenessAnalysisInfo& LivenessAnalysis::getLivenessInfo() const
{
    return m_blockLivenessInfo;
}

LivenessAnalysis::LivenessAnalysisInfo& LivenessAnalysis::getLivenessInfo()
{
    return m_blockLivenessInfo;
}

const BasicBlockLivenessInfo& LivenessAnalysis::getLivenessInfo(llvm::BasicBlock* block) const
{
    auto pos = m_blockLivenessInfo.find(block);
    return pos->second;
}

BasicBlockLivenessInfo& LivenessAnalysis::getLivenessInfo(llvm::BasicBlock* block)
{
    auto pos = m_blockLivenessInfo.find(block);
    return pos->second;
}

/*
   function DAG_DFS(block B)
   for each S ∈ CFG_succs(B) such that (B, S) is not a loop-edge do
        if S not processed then DAG_DFS(S)
   Live = PhiUses(B)
   for each S ∈ CFG_succs(B) such that (B, S) is not a loop-edge do
        Live = Live ∪ (LiveIn(S) − PhiDefs(S))
   LiveOut(B) = Live
   for each program point p in B, backward do
        remove variables defined at p from Live
        add uses at p in Live
   LiveIn(B) = Live ∪ PhiDefs(B)
   mark B as processed
 */
void LivenessAnalysis::dag_dfs(llvm::BasicBlock* B)
{
    auto res = m_blockLivenessInfo.insert(std::make_pair(B, BasicBlockLivenessInfo(B)));
    assert(res.second);
    auto& blockInfo = res.first->second;
    update_block_phi_defs(B, blockInfo);

    auto it = succ_begin(B);
    while (it != succ_end(B)) {
        llvm::BasicBlock* S = *it;
        if (is_loop_edge(B, S)) {
            ++it;
            continue;
        }
        if (m_unprocessedBlocks.find(S) != m_unprocessedBlocks.end()) {
            update_block_phi_uses_from_successor(blockInfo, S);
            dag_dfs(S);
        }
        ++it;
    }
    BasicBlockLivenessInfo::ValueSet live = blockInfo.get_phiUses();
    it = succ_begin(B);
    while (it != succ_end(B)) {
        llvm::BasicBlock* S = *it;
        ++it;
        if (is_loop_edge(B, S)) {
            continue;
        }
        auto S_info = m_blockLivenessInfo.find(S);
        assert(S_info != m_blockLivenessInfo.end());
        auto S_liveIns = S_info->second.get_liveIn();
        auto S_phiDefs = S_info->second.get_phiDefs();
        std::for_each(S_phiDefs.begin(), S_phiDefs.end(),
                     [&S_liveIns] (llvm::Value* val) { S_liveIns.erase(val);});
        live.insert(S_liveIns.begin(), S_liveIns.end());
    }
    blockInfo.set_liveOut(live);
    update_block_liveins(B, live);
    blockInfo.set_liveIn(live);
    m_unprocessedBlocks.erase(B);
}


/*
   function LoopTree_DFS(node N of the loop forest)
   if N is a loop node then
        Let BN = Block(N) ⊲ The loop-header of N
        Let LiveLoop = LiveIn(B) − PhiDefs(BN )
        for each M ∈ LoopTree_succs(N) do
            Let BM = Block(M) ⊲ The loop-header or basic block of M
            LiveIn(BM) = LiveIn(BM) ∪ LiveLoop
            LiveOut(BM) = LiveOut(BM) ∪ LiveLoop
            LoopTree_DFS(M)
*/
void LivenessAnalysis::looptree_dfs(llvm::BasicBlock* B)
{
    auto loop = m_LI->getLoopFor(B);
    if (!loop || loop->getHeader() != B) {
        return;
    }
    auto block_info = m_blockLivenessInfo.find(B);
    assert(block_info != m_blockLivenessInfo.end());
    BasicBlockLivenessInfo::ValueSet livein_loop = block_info->second.get_liveIn();
    std::for_each(block_info->second.get_phiDefs().begin(), block_info->second.get_phiDefs().end(),
                  [&livein_loop] (llvm::Value* val) { livein_loop.erase(val); });
    for (auto C : loop->getBlocks()) {
        auto child_info = m_blockLivenessInfo.find(C);
        assert(child_info != m_blockLivenessInfo.end());
        child_info->second.add_liveIn(livein_loop);
        child_info->second.add_liveOut(livein_loop);
        auto child_loop = m_LI->getLoopFor(C);
        if (child_loop != loop && child_loop->getHeader() == C) {
            looptree_dfs(C);
        }
    }
}

/*
    edge (S, E) in CFG is loop edge, if both S and E are in the same loop, and E is the header of that loop
*/
bool LivenessAnalysis::is_loop_edge(llvm::BasicBlock* S, llvm::BasicBlock* E) const
{
    auto SL = m_LI->getLoopFor(S);
    if (SL) {
        return (SL->getHeader() == E);
    }
    return false;
}

void LivenessAnalysis::dump(llvm::Function& F)
{
    llvm::dbgs() << "**** " << F.getName() << " ****\n";
    for (auto& B : F) {
        auto info = m_blockLivenessInfo.find(&B);
        if (info == m_blockLivenessInfo.end()) {
            continue;
        }
        //assert(info != m_blockLivenessInfo.end());
        llvm::dbgs() << "BB: " << B.getName() << "\n";
        const auto& liveins = info->second.get_liveIn();
        llvm::dbgs() << "Live in values: " << liveins.size() << "\n";
        for (const auto& val : liveins) {
            llvm::dbgs() << "   " << *val << "\n";
        }
        llvm::dbgs() << "\n";
        const auto& liveouts = info->second.get_liveOut();
        llvm::dbgs() << "Live out values: " << liveouts.size() << "\n";
        for (const auto& val : liveouts) {
            llvm::dbgs() << "   " << *val << "\n";
        }
        llvm::dbgs() << "----------\n";
    }
}

char LivenessAnalysis::ID = 0;
static llvm::RegisterPass<LivenessAnalysis> X("live","Liveness Analysis pass");

}

