#include "Debug/ILPOptimizationData.h"

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

namespace {

int getFunctionSize(llvm::Function& F)
{
    int size = 0;
    for (auto& B : F) {
        size += B.size();
    }
    return size;
}
} // unnamed namespace

namespace debug {

ILPOptimizationDataRow::ILPOptimizationDataRow(llvm::Function* F,
                                               pdg::PDG* pdg,
                                               llvm::LoopInfo* loopInfo)
    : m_F(F)
    , m_pdg(pdg)
    , m_loopInfo(loopInfo)
{
}

int ILPOptimizationDataRow::getCallNum(llvm::Function* F) const
{
    auto pos = m_contextSwitchNum.find(F);
    if (pos == m_contextSwitchNum.end()) {
        return 0;
    } 
    return pos->second;
}

void ILPOptimizationDataRow::fillRow()
{
    if (!m_pdg->hasFunctionPDG(m_F)) {
        return;
    }
    m_Fsize = getFunctionSize(*m_F);
    auto Fpdg = m_pdg->getFunctionPDG(m_F);
    for (auto& B : *m_F) {
        for (auto& I : B ) {
            if (!llvm::isa<llvm::CallInst>(&I)
                    && !llvm::isa<llvm::InvokeInst>(&I)) {
                continue;
            }
            const bool isInLoop = (m_loopInfo->getLoopFor(&B) != nullptr);
            auto pdgNode = Fpdg->getNode(&I);
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
                    if (isInLoop) {
                        m_contextSwitchNum[functionNode->getFunction()] = -1;
                    } else {
                        ++m_contextSwitchNum[functionNode->getFunction()];
                    }
                }
            }
        }
    }
}

std::string ILPOptimizationDataRow::getRowAsString() const
{
    std::stringstream rowStr;
    rowStr << m_F->getName().str() << " |";
    if (m_F->isDeclaration()) {
        rowStr << "\n---------------------------------------------\n";
        return rowStr.str();
    }
    rowStr << m_Fsize << " |";
    for (const auto& contextSwitch : m_contextSwitchNum) {
        rowStr << contextSwitch.first->getName().str() << " " << contextSwitch.second << "; ";
    }
    rowStr << "\n---------------------------------------------\n";
    return rowStr.str();
}

ILPOptimizationData::ILPOptimizationData(llvm::Module& M,
                                         pdg::PDG* pdg,
                                         const LoopInfoGetter& loopInfoGetter)
    : m_M(M)
    , m_pdg(pdg)
    , m_loopInfoGetter(loopInfoGetter)
{
}

void ILPOptimizationData::collectOptimizationData()
{
    int idx = 0;
    for (auto& F : m_M) {
        m_functions.insert(std::make_pair(&F, idx++));
        if (F.isDeclaration()) {
            m_ilpOptData.push_back(ILPOptimizationDataRow(&F, m_pdg, nullptr));
            continue;
        }
        llvm::LoopInfo* loop = m_loopInfoGetter(&F);
        m_ilpOptData.push_back(ILPOptimizationDataRow(&F, m_pdg, loop));
        m_ilpOptData.back().fillRow();
    }
}

} // namespace debug

