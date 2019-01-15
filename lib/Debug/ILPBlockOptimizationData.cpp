#include "Debug/ILPBlockOptimizationData.h"

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

#include "PDG/PDG/PDG.h"
#include "PDG/PDG/FunctionPDG.h"
#include "PDG/PDG/PDGEdge.h"
#include "PDG/PDG/PDGNode.h"
#include "PDG/PDG/PDGLLVMNode.h"

#include <sstream>

namespace debug {

ILPBlockOptimizationDataRow::ILPBlockOptimizationDataRow(
        llvm::BasicBlock* B,
        pdg::FunctionPDG* pdg,
        llvm::Loop* loop)
    : m_block(B)
    , m_fPdg(pdg)
    , m_loop(loop)
{
}

llvm::Function* ILPBlockOptimizationDataRow::getFunction() const
{
    return m_block->getParent();
}

int ILPBlockOptimizationDataRow::getBSize() const
{
    return m_block->size();
}
int ILPBlockOptimizationDataRow::getCallNum(llvm::Function* F) const
{
    auto pos = m_contextSwitchNum.find(F);
    if (pos == m_contextSwitchNum.end()) {
        return 0;
    } 
    return pos->second;
}

void ILPBlockOptimizationDataRow::fillRow()
{
    for (auto& I : *m_block ) {
        if (!llvm::isa<llvm::CallInst>(&I)
                && !llvm::isa<llvm::InvokeInst>(&I)) {
            continue;
        }
        auto pdgNode = m_fPdg->getNode(&I);
        for (auto edge_it = pdgNode->outEdgesBegin();
             edge_it != pdgNode->outEdgesEnd();
             ++edge_it) {
            if (!(*edge_it)->isControlEdge()) {
                continue;
            }
            if (auto* functionNode = llvm::dyn_cast<pdg::PDGLLVMFunctionNode>((*edge_it)->getDestination().get())) {
                if (m_contextSwitchNum[functionNode->getFunction()] == -1) {
                    continue;
                }
                if (m_loop) {
                    m_contextSwitchNum[functionNode->getFunction()] = -1;
                } else {
                    ++m_contextSwitchNum[functionNode->getFunction()];
                }
            }
        }
    }
}

std::string ILPBlockOptimizationDataRow::getRowAsString() const
{
    std::stringstream rowStr;
    rowStr << m_block->getParent()->getName().str() << "    " << m_block->getName().str() << " |";
    rowStr << m_block->size() << " |";
    for (const auto& contextSwitch : m_contextSwitchNum) {
        rowStr << contextSwitch.first->getName().str() << " " << contextSwitch.second << "; ";
    }
    rowStr << "\n---------------------------------------------\n";
    return rowStr.str();
}

ILPBlockOptimizationData::ILPBlockOptimizationData(llvm::Module& M,
                                                   pdg::PDG* pdg,
                                                   const LoopInfoGetter& loopInfoGetter)
    : m_M(M)
    , m_pdg(pdg)
    , m_loopInfoGetter(loopInfoGetter)
{
}

void ILPBlockOptimizationData::collectOptimizationData()
{
    int idx = 0;
    for (auto& F : m_M) {
        m_functions.insert(std::make_pair(&F, idx++));
        if (F.isDeclaration()) {
            continue;
        }
        llvm::LoopInfo* loopInfo = m_loopInfoGetter(&F);
        for (auto& B : F) {
            auto* loop = loopInfo->getLoopFor(&B);
            auto* fPdg = m_pdg->getFunctionPDG(&F).get();
            auto pos = m_ilpOptData.insert(std::make_pair(&B, ILPBlockOptimizationDataRow(&B, fPdg, loop)));
            pos.first->second.fillRow();
        }
    }
}

} // namespace debug

