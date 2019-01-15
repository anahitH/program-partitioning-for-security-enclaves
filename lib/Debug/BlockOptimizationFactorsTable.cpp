#include "Debug/ILPBlockOptimizationData.h"

#include "Analysis/ProgramPartitionAnalysis.h"
#include "Analysis/LivenessAnalysis.h"

#include "llvm/Pass.h"
#include "llvm/Analysis/LoopInfo.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/ValueMap.h"
#include "llvm/Pass.h"
#include "llvm/PassRegistry.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Transforms/IPO/PassManagerBuilder.h"
#include "llvm/Transforms/Utils/Cloning.h"
#include "llvm/Transforms/Utils/ValueMapper.h"

#include "PDG/Passes/PDGBuildPasses.h"
#include "PDG/PDG/PDG.h"
#include "PDG/PDG/FunctionPDG.h"
#include "PDG/PDG/PDGEdge.h"
#include "PDG/PDG/PDGNode.h"
#include "PDG/PDG/PDGLLVMNode.h"

#include <fstream>
#include <functional>
#include <list>
#include <sstream>
#include <unordered_map>

namespace debug {

class GroupBlocks
{
public:
    using BlockGroup = std::unordered_set<llvm::BasicBlock*>;
    using BlockGroups = std::vector<BlockGroup>;
    using BlockGroupIndex = std::unordered_map<llvm::BasicBlock*, int>;

    using LoopInfoGetter = std::function<llvm::LoopInfo* (llvm::Function*)>;
    using LivenessInfoGetter = std::function<vazgen::LivenessAnalysis* (llvm::Function*)>;

public:
    GroupBlocks(llvm::Module& M,
                const LoopInfoGetter& loopGetter,
                const LivenessInfoGetter& livenessInfoGetter,
                pdg::PDG* pdg)
        : m_M(M)
        , m_pdg(pdg)
        , m_loopInfoGetter(loopGetter)
        , m_livenessInfoGetter(livenessInfoGetter)
    {
    }

    void computeBlockGroups();

    const BlockGroups& getBlockGroups() const
    {
        return m_groups;
    }

    int getBlockGroupIdx(llvm::BasicBlock* B) const
    {
        auto pos = m_blockGroupIdx.find(B);
        if (pos == m_blockGroupIdx.end()) {
            llvm::dbgs() << B->getParent()->getName() << "  " << B->getName() << "\n";
        }
        assert(pos != m_blockGroupIdx.end());
        return pos->second;
    }

    void dumpGroups() const;

private:
    void computeLoopBlocks();
    void computeLivenessGroups();
    void assignGroup(const BlockGroup& group);
    void assignGroup(llvm::BasicBlock* block1, llvm::BasicBlock* block2);
    void assignGroup(llvm::BasicBlock* block);
    int mergeGroups(int idx1, int idx2);
    bool areDependent(const vazgen::BasicBlockLivenessInfo::ValueSet& liveOuts,
                      const vazgen::BasicBlockLivenessInfo::ValueSet& liveIns) const;
    bool isReachableFromArgument(llvm::Value* value) const;

private:
    llvm::Module& m_M;
    pdg::PDG* m_pdg;
    BlockGroups m_groups;
    BlockGroupIndex m_blockGroupIdx;
    const LoopInfoGetter& m_loopInfoGetter;
    const LivenessInfoGetter& m_livenessInfoGetter;
}; // class GroupBlocks

void GroupBlocks::computeBlockGroups()
{
    computeLoopBlocks();
    computeLivenessGroups();
}

void GroupBlocks::dumpGroups() const
{
    for (const auto& group : m_groups) {
        if (group.empty()) {
            continue;
        }
        if (auto* block = *group.begin()) {
            llvm::dbgs() << "Function " << block->getParent()->getName() << "\n";
        }
        for (const auto& block : group) {
            llvm::dbgs() << "  " << block->getName();
        }
        llvm::dbgs() << "\n";
    }
}

void GroupBlocks::computeLoopBlocks()
{
    for (auto& F : m_M) {
        if (F.isDeclaration()) {
            continue;
        }
        auto* loopInfo = m_loopInfoGetter(&F);
        for (auto& B : F) {
            auto* loop = loopInfo->getLoopFor(&B);
            if (!loop) {
                continue;
            }
            BlockGroup group;
            group.insert(&B);
            const auto& blocks = loop->getBlocks();
            for (auto block : blocks) {
                group.insert(block);
            }
            assignGroup(group);
        }
    }
}

void GroupBlocks::computeLivenessGroups()
{
    for (auto& F : m_M) {
        if (F.isDeclaration()) {
            continue;
        }
        auto* liveness = m_livenessInfoGetter(&F);
        for (auto& block : F) {
            const auto& blockLiveOuts = liveness->getLivenessInfo(&block).get_liveOut();
            if (llvm::succ_empty(&block)) {
                auto res = m_blockGroupIdx.insert(std::make_pair(&block, m_groups.size()));
                if (res.second) {
                    m_groups.push_back(BlockGroup());
                    m_groups.back().insert(&block);
                }
                continue;
            }
            for (auto succ_it = llvm::succ_begin(&block);
                 succ_it != llvm::succ_end(&block);
                 ++succ_it) {
                llvm::BasicBlock* succ_block = *succ_it;
                const auto& succLiveIns = liveness->getLivenessInfo(succ_block).get_liveIn();
                if (areDependent(blockLiveOuts, succLiveIns)) {
                    assignGroup(&block, succ_block);
                } else {
                    assignGroup(&block);
                    assignGroup(succ_block);
                }
            }
        }
    }
}

void GroupBlocks::assignGroup(const BlockGroup& group)
{
    int idx = -1;
    for (auto& B : group) {
        auto idxPos = m_blockGroupIdx.find(B);
        if (idxPos == m_blockGroupIdx.end()) {
            continue;
        }
        if (idx != -1) {
            assert(idx == idxPos->second);
        } else {
            idx = idxPos->second;
        }
    }
    if (idx == -1) {
        m_groups.push_back(group);
        idx = m_groups.size() - 1;
    } else {
        m_groups[idx].insert(group.begin(), group.end());
    }
    for (auto& B : group) {
        m_blockGroupIdx[B] = idx;
    }
}

void GroupBlocks::assignGroup(llvm::BasicBlock* block1, llvm::BasicBlock* block2)
{
    int idx = -1;
    auto idx_pos1 = m_blockGroupIdx.find(block1);
    auto idx_pos2 = m_blockGroupIdx.find(block2);
    if (idx_pos1 != m_blockGroupIdx.end()
            && idx_pos2 != m_blockGroupIdx.end()) {
        if (idx_pos1->second == idx_pos2->second) {
            return;
        }
        idx = mergeGroups(idx_pos1->second, idx_pos2->second);
        m_groups[idx].insert(block1);
        m_groups[idx].insert(block2);
        m_blockGroupIdx[block1] = idx;
        m_blockGroupIdx[block2] = idx;
        return;
    }
    if (idx_pos1 != m_blockGroupIdx.end()) {
        idx = idx_pos1->second;
    } else if (idx_pos2 != m_blockGroupIdx.end()) {
        idx = idx_pos2->second;
    } else {
        idx = m_groups.size();
        m_groups.push_back(BlockGroup());
    }
    m_groups[idx].insert(block1);
    m_groups[idx].insert(block2);
    m_blockGroupIdx[block1] = idx;
    m_blockGroupIdx[block2] = idx;
}

void GroupBlocks::assignGroup(llvm::BasicBlock* block)
{
    auto res = m_blockGroupIdx.insert(std::make_pair(block, m_groups.size()));
    if (res.second) {
        m_groups.push_back(BlockGroup());
        m_groups.back().insert(block);
    }
}

int GroupBlocks::mergeGroups(int idx1, int idx2)
{
    int idx = std::min(idx1, idx2);
    int remove_idx = std::max(idx1, idx2);
    for (auto* block : m_groups[remove_idx]) {
        m_groups[idx].insert(block);
        m_blockGroupIdx[block] = idx;
    }
    m_groups[remove_idx].clear();
    return idx;
}

bool GroupBlocks::areDependent(
                      const vazgen::BasicBlockLivenessInfo::ValueSet& liveOuts,
                      const vazgen::BasicBlockLivenessInfo::ValueSet& liveIns) const
{
    for (auto val : liveOuts) {
        if (liveIns.find(val) != liveIns.end()) {
            if (!isReachableFromArgument(val)) {
                return true;
            }
        }
    }
    return false;
}

bool GroupBlocks::isReachableFromArgument(llvm::Value* value) const
{
    auto* instr = llvm::dyn_cast<llvm::Instruction>(value);
    if (!instr) {
        return false;
    }
    llvm::Function* F = instr->getFunction();
    auto Fpdg = m_pdg->getFunctionPDG(F);
    assert(Fpdg);
    std::list<llvm::Value*> workingList;
    workingList.push_back(value);
    std::unordered_set<llvm::Value*> processed_values;
    while (!workingList.empty()) {
        llvm::Value* currentVal = workingList.back();
        workingList.pop_back();
        if (!processed_values.insert(currentVal).second) {
            continue;
        }
        if (!Fpdg->hasNode(currentVal)) {
            continue;
        }
        auto node = Fpdg->getNode(currentVal);
        for (auto out_it = node->outEdgesBegin();
             out_it != node->outEdgesEnd();
             ++out_it) {
            if ((*out_it)->isControlEdge()) {
                continue;
            }
            auto llvmNode = llvm::dyn_cast<pdg::PDGLLVMNode>((*out_it)->getDestination().get());
            if (!llvmNode) {
                continue;
            }
            if (!llvmNode->getNodeValue()) {
                continue;
            }
            if (auto* store = llvm::dyn_cast<llvm::StoreInst>(llvmNode->getNodeValue())) {
                if (llvm::dyn_cast<llvm::Argument>(store->getValueOperand())) {
                    return true;
                }
            }
            workingList.push_back(llvmNode->getNodeValue());
        }
    }
    return false;
}

bool isSecureGroup(const GroupBlocks::BlockGroup& group, const vazgen::Partition& partition)
{
    if (group.empty()) {
        return false;
    }
    llvm::BasicBlock* block = *group.begin();
    auto pos = partition.getSecureBlocks().find(block->getParent());
    if (pos == partition.getSecureBlocks().end()) {
        return false;
    }
    const auto& secureBlocks = pos->second;
    if (secureBlocks.empty()) {
        return false;
    }
    for (auto& B : group) {
        if (secureBlocks.find(B) != secureBlocks.end()) {
            return true;
        }
    }
    return false;
}


class BlockOptimizationFactorsTablePrinterPass : public llvm::ModulePass
{
public:
    static char ID;

    BlockOptimizationFactorsTablePrinterPass()
        : llvm::ModulePass(ID)
    {
    }

public:
    void getAnalysisUsage(llvm::AnalysisUsage& AU) const override;
    bool runOnModule(llvm::Module& M) override;

private:
    void dumpTable() const;
    void dumpILPModule(const std::string& filename) const;
    std::vector<std::string> getBlockGroupVariableNames() const;
    std::unordered_map<llvm::Function*, std::string> getFunctionVariableNames() const;
    std::vector<ILPBlockOptimizationDataRow::ContextSwitchData> getGroupContextSwitches() const;
    std::vector<int> getGroupsSizes() const;

private:
    ILPBlockOptimizationData* m_ilpData;
    GroupBlocks* m_groupBlocks;
    llvm::Module* m_M;
}; // class OptimizationFactorsTablePrinterPass

void BlockOptimizationFactorsTablePrinterPass::getAnalysisUsage(llvm::AnalysisUsage& AU) const
{
    AU.setPreservesCFG();
    AU.addRequired<vazgen::LivenessAnalysis>();
    AU.addRequired<pdg::SVFGPDGBuilder>();
    AU.addRequired<llvm::LoopInfoWrapperPass>();
    AU.addRequired<vazgen::ProgramPartitionAnalysis>();
    AU.setPreservesAll();
}

bool BlockOptimizationFactorsTablePrinterPass::runOnModule(llvm::Module& M)
{
    m_M = &M;
    auto pdg = getAnalysis<pdg::SVFGPDGBuilder>().getPDG();
    const auto& loopGetter = [this] (llvm::Function* F)
        {   return &this->getAnalysis<llvm::LoopInfoWrapperPass>(*F).getLoopInfo(); };
    const auto& livenessGetter = [this] (llvm::Function* F)
        { return &this->getAnalysis<vazgen::LivenessAnalysis>(*F); };

    m_groupBlocks = new GroupBlocks(M, loopGetter, livenessGetter, pdg.get());
    m_groupBlocks->computeBlockGroups();
    m_groupBlocks->dumpGroups();

    m_ilpData = new ILPBlockOptimizationData(M, pdg.get(), loopGetter);
    m_ilpData->collectOptimizationData();

    dumpTable();
    dumpILPModule(M.getName().str() + ".lp");

    delete m_ilpData;
    delete m_groupBlocks;

    return false;
}

void BlockOptimizationFactorsTablePrinterPass::dumpTable() const
{
    const auto& groupsContextSwitches = getGroupContextSwitches();
    const auto& groupsSizes = getGroupsSizes();
    const auto& blockGroups = m_groupBlocks->getBlockGroups();

    std::ofstream tableFile("opt_table.txt");
    for (int i = 0; i < blockGroups.size(); ++i) {
        if (blockGroups[i].empty()) {
            continue;
        }
        tableFile << "group" << i << ": ";
        tableFile << " function " << (*blockGroups[i].begin())->getParent()->getName().str() << ":  ";
        for (auto B : blockGroups[i]) {
            tableFile << B->getName().str() << "   ";
        }
        tableFile << "| " << groupsSizes[i] << " |";
        for (const auto& ctxSwitchData : groupsContextSwitches[i]) {
            tableFile << ctxSwitchData.first->getName().str() << " " << ctxSwitchData.second << "; ";
        }
        tableFile << "\n";
    }
    tableFile.close();
}

void BlockOptimizationFactorsTablePrinterPass::dumpILPModule(const std::string& filename) const
{
    const auto& functions = m_ilpData->getFunctions();
    const auto& tableRows = m_ilpData->getILPBlockOptimizationData();
    const auto& securePart = getAnalysis<vazgen::ProgramPartitionAnalysis>().getProgramPartition().getSecurePartition();

    std::ofstream modelFile(filename);
    std::ofstream helperFile("variable_names.txt");
    std::unordered_map<llvm::Function*, std::string> variableNames;
    std::stringstream objectiveStr;
    std::stringstream constraintsStr;
    std::stringstream boundsStr;
    objectiveStr << "Maximize\n";
    constraintsStr << "Subject To\n";
    boundsStr << "Bounds\n";

    int edgeNum = 0;
    const auto& blockVarNames = getBlockGroupVariableNames();
    auto functionVarNames = getFunctionVariableNames();
    const auto& blockGroups = m_groupBlocks->getBlockGroups();
    const auto& groupsContextSwitches = getGroupContextSwitches();
    const auto& groupsSizes = getGroupsSizes();

    for (const auto& declFVar : functionVarNames) {
        constraintsStr << declFVar.second << " <= 0\n";
        boundsStr << "0 <= " << declFVar.second << " <= 1\n";
    }

    for (int i = 0; i < blockGroups.size(); ++i) {
        const auto& currentG = blockGroups[i];
        const std::string varName = blockVarNames[i];
        if (currentG.empty()) {
            continue;
        }
        helperFile << "group" << i << "  " << varName << "\n";
        llvm::Function* currentGF = (*currentG.begin())->getParent();
        if (currentGF->getName() == "main"
                && currentG.find(&currentGF->getEntryBlock()) != currentG.end()) {
            constraintsStr << blockVarNames[i] << " <= 0\n";
        } else if (isSecureGroup(currentG, securePart)) {
            boundsStr << "1 <= " << blockVarNames[i] << " <= 1\n";
        } else {
            boundsStr << "0 <= " << blockVarNames[i] << " <= 1\n";
        }
        const auto& groupContextSwitches = groupsContextSwitches[i];
        for (const auto& ctxSwitch : groupContextSwitches) {
            llvm::Function* F = ctxSwitch.first;
            std::string edgeVarName;
            std::string calleeVarName;
            if (F->isDeclaration()) {
                edgeVarName = "b" + std::to_string(i) + functionVarNames[F].back();
                calleeVarName = functionVarNames[F];
                helperFile << "edge group" << i << "  to" << F->getName().str() << "  " << edgeVarName << "\n";
            } else {
                llvm::BasicBlock* entryBlock = &F->getEntryBlock();
                int blockGroupIdx = m_groupBlocks->getBlockGroupIdx(entryBlock);
                calleeVarName = blockVarNames[blockGroupIdx];
                edgeVarName = "d" + std::to_string(i) + std::to_string(blockGroupIdx);
                helperFile << "edge group" << i << "  to" << blockGroupIdx << "  " << edgeVarName << "\n";
            }
            if (ctxSwitch.second == -1) {
                objectiveStr << "100 ";
            } else {
                objectiveStr << std::to_string(ctxSwitch.second) << " ";
            }
            ++edgeNum;
            objectiveStr << edgeVarName << " + ";
            boundsStr << "0 <= " << edgeVarName << " <= 1\n";
            constraintsStr << edgeVarName << " - " << varName << " + " << calleeVarName << " <= 1\n";
            constraintsStr << edgeVarName << " + " << varName << " - " << calleeVarName << " <= 1\n";
        }
        for (auto& B : currentG) {
            int Bidx = m_groupBlocks->getBlockGroupIdx(B);
            const auto& BvarName = blockVarNames[Bidx];
            for (auto succ_it = llvm::succ_begin(B); succ_it != llvm::succ_end(B); ++succ_it) {
                llvm::BasicBlock* succ = *succ_it;
                if (currentG.find(succ) == currentG.end()) {
                    int succIdx = m_groupBlocks->getBlockGroupIdx(succ);
                    const auto& succVarName = blockVarNames[succIdx];
                    std::string edgeVarName = "l" + std::to_string(Bidx) + std::to_string(succIdx);
                    helperFile << "edge local group" << i << "  to " << succIdx << "  " << edgeVarName << "\n";
                    constraintsStr << edgeVarName << " - " << BvarName << " + " << succVarName << " <= 1\n";
                    constraintsStr << edgeVarName << " + " << BvarName << " - " << succVarName << " <= 1\n";
                    objectiveStr << edgeVarName << " + ";
                    boundsStr << "0 <= " << edgeVarName << " <= 1\n";
                    ++edgeNum;

                }
            }
        }
    }
    std::string obj = objectiveStr.str();
    obj.pop_back();
    obj.pop_back();
    objectiveStr = std::stringstream();
    objectiveStr << obj; 
    for (int i = 0; i < blockGroups.size(); ++i) {
        const auto& group = blockGroups[i];
        if (group.empty()) {
            continue;
        }
        objectiveStr << " - 100 " << std::to_string(groupsSizes[i]) << " " << blockVarNames[i];
    }
    objectiveStr << "\n";
    modelFile << objectiveStr.str();
    modelFile << constraintsStr.str();
    modelFile << boundsStr.str();
    modelFile << "end\n";

    modelFile.close();
    helperFile.close();
    llvm::dbgs() << "Edge num " << edgeNum << "\n";
}

std::vector<std::string>
BlockOptimizationFactorsTablePrinterPass::getBlockGroupVariableNames() const
{
    std::vector<std::string> names;
    for (int i = 0; i < m_groupBlocks->getBlockGroups().size(); ++i) {
        names.push_back("g" + std::to_string(i));
    }
    return names;
}

std::unordered_map<llvm::Function*, std::string>
BlockOptimizationFactorsTablePrinterPass::getFunctionVariableNames() const
{
    std::unordered_map<llvm::Function*, std::string> functionVarNames;
    int fidx = 0;
    for (auto& F : *m_M) {
        if (F.isDeclaration()) {
            functionVarNames.insert(std::make_pair(&F, "f"+std::to_string(fidx++)));
        }
    }
    return functionVarNames;
}

std::vector<ILPBlockOptimizationDataRow::ContextSwitchData>
BlockOptimizationFactorsTablePrinterPass::getGroupContextSwitches() const
{
    const auto& groups = m_groupBlocks->getBlockGroups();
    std::vector<ILPBlockOptimizationDataRow::ContextSwitchData> contextSwitchData(groups.size());
    for (int i = 0; i < groups.size(); ++i) {
        auto& groupCtxSwitchData = contextSwitchData[i];
        for (auto& B : groups[i]) {
            const auto& BctxSwData = m_ilpData->getILPBlockOptimizationData(const_cast<llvm::BasicBlock*>(B));
            for (const auto& data : BctxSwData.getContextSwitchData()) {
                if (groupCtxSwitchData[data.first]
                        || data.second == -1) {
                    groupCtxSwitchData[data.first] = -1;
                } else {
                    groupCtxSwitchData[data.first] += data.second;
                }
            }
        }
    }
    return contextSwitchData;
}

std::vector<int> BlockOptimizationFactorsTablePrinterPass::getGroupsSizes() const
{
    const auto& groups = m_groupBlocks->getBlockGroups();
    std::vector<int> sizes(groups.size());
    for (int i = 0; i < groups.size(); ++i) {
        if (groups[i].empty()) {
            continue;
        }
        int& size = sizes[i];
        for (auto& B : groups[i]) {
            size += B->size();
        }
    }
    return sizes;
}

char BlockOptimizationFactorsTablePrinterPass::ID = 0;
static llvm::RegisterPass<BlockOptimizationFactorsTablePrinterPass> X("dump-block-opt-table","Dump Optimization table in block level");


} // namespace debug

