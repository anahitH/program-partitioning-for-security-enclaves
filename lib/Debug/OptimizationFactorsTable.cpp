#include "Debug/ILPOptimizationData.h"

#include "Analysis/ProgramPartitionAnalysis.h"

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

#include <unordered_map>
#include <sstream>
#include <fstream>

namespace debug {

namespace {

double getContextSwitchCoef(const std::vector<ILPOptimizationDataRow>& tableRows)
{
    int functions_size = 0;
    int functions_num = 0;
    for (const auto& item : tableRows) {
        if (item.getFunction()->isDeclaration()) {
            continue;
        }
        ++functions_num;
        functions_size += item.getFSize();
    }
    return (functions_size * 1.0) / functions_num;
}

}

class OptimizationFactorsTablePrinterPass : public llvm::ModulePass
{
public:
    static char ID;

    OptimizationFactorsTablePrinterPass()
        : llvm::ModulePass(ID)
    {
    }

public:
    void getAnalysisUsage(llvm::AnalysisUsage& AU) const override;
    bool runOnModule(llvm::Module& M) override;

private:
    void dumpTable() const;
    void dumpILPModule(const std::string& filename) const;

private:
    ILPOptimizationData* m_ilpData;
}; // class OptimizationFactorsTablePrinterPass

void OptimizationFactorsTablePrinterPass::getAnalysisUsage(llvm::AnalysisUsage& AU) const
{
    AU.addRequired<pdg::SVFGPDGBuilder>();
    AU.addRequired<llvm::LoopInfoWrapperPass>();
    AU.addRequired<vazgen::ProgramPartitionAnalysis>();
    AU.setPreservesAll();
}

bool OptimizationFactorsTablePrinterPass::runOnModule(llvm::Module& M)
{
    auto pdg = getAnalysis<pdg::SVFGPDGBuilder>().getPDG();
    const auto& loopGetter = [this] (llvm::Function* F)
        {   return &this->getAnalysis<llvm::LoopInfoWrapperPass>(*F).getLoopInfo(); };

    m_ilpData = new ILPOptimizationData(M, pdg.get(), loopGetter);
    m_ilpData->collectOptimizationData();
    dumpTable();
    dumpILPModule(M.getName().str() + ".lp");
    delete m_ilpData;

    return false;
}

void OptimizationFactorsTablePrinterPass::dumpTable() const
{
    const auto& functions = m_ilpData->getFunctions();
    const auto& tableRows = m_ilpData->getILPOptimizationData();

   std::ofstream tableFile("opt_table.txt");
    for (int i = 0; i < tableRows.size(); ++i) {
        tableFile << i << ") " << tableRows[i].getRowAsString();
    }
    tableFile << "   ";
    for (int i = 0; i < tableRows.size(); ++i) {
        tableFile << i << " *";
    }
    tableFile << "\n";
    for (int i = 0; i < 2 * tableRows.size(); ++i) {
        tableFile << " *";
    }
    tableFile << "\n";
    for (int i = 0; i < tableRows.size(); ++i) {
        tableFile << i << " *";
        for (auto& f : functions) {
            tableFile << tableRows[i].getCallNum(f.first) << " |";
        }
        tableFile << "\n";
        for (int i = 0; i < tableRows.size(); ++i) {
            tableFile << " ---";
        }
        tableFile << "\n";
    }
    tableFile.close();
}

void OptimizationFactorsTablePrinterPass::dumpILPModule(const std::string& filename) const
{
    const auto& functions = m_ilpData->getFunctions();
    const auto& tableRows = m_ilpData->getILPOptimizationData();

    const auto& securePart = getAnalysis<vazgen::ProgramPartitionAnalysis>().getProgramPartition().getSecurePartition();
    double coef = 1; // getContextSwitchCoef(tableRows);

    std::ofstream modelFile(filename);
    std::unordered_map<llvm::Function*, std::string> variableNames;
    std::stringstream objectiveStr;
    std::stringstream constraintsStr;
    std::stringstream boundsStr;
    objectiveStr << "Maximize\n";
    constraintsStr << "Subject To\n";
    boundsStr << "Bounds\n";
    for (const auto& fRow : tableRows) {
        llvm::Function* currentF = fRow.getFunction();
        int currentIdx = functions.find(currentF)->second;
        auto res = variableNames.insert(std::make_pair(currentF, "v" + std::to_string(currentIdx)));
        std::string varName = res.first->second;
        if (currentF->isDeclaration() || currentF->getName() == "main") {
            constraintsStr << varName << "<= 0\n";
        } else if (securePart.contains(currentF)) {
            boundsStr << "1 <= " << varName << " <= 1\n";
        } else {
            boundsStr << "0 <= " << varName << " <= 1\n";
        }
        const auto& contextSwitches = fRow.getContextSwitchData();
        for (const auto& calleeItem : contextSwitches) {
            llvm::Function* callee = calleeItem.first;
            auto calleeIdx = functions.find(callee)->second;
            auto nameRes = variableNames.insert(std::make_pair(callee, "v" + std::to_string(calleeIdx)));
            std::string calleeVarName = nameRes.first->second;
            std::string edgeVarName = "f" + std::to_string(currentIdx) + std::to_string(calleeIdx);
            if (calleeItem.second == -1) {
                objectiveStr << "100 ";
            } else {
                objectiveStr << std::to_string(calleeItem.second) << " ";
            }
            objectiveStr << std::to_string(coef) << " " << edgeVarName << " + ";
            constraintsStr << edgeVarName << " - " << varName << " + " << calleeVarName << " <= 1\n";
            constraintsStr << edgeVarName << " + " << varName << " - " << calleeVarName << " <= 1\n";
        }
    }
    std::string obj = objectiveStr.str();
    obj.pop_back();
    obj.pop_back();
    objectiveStr = std::stringstream();
    objectiveStr << obj; 
    for (const auto& fRow : tableRows) {
        llvm::Function* currentF = fRow.getFunction();
        if (currentF->isDeclaration()) {
            continue;
        }
        std::string varName = variableNames.find(currentF)->second;
        objectiveStr << " - 100 " << std::to_string(fRow.getFSize()) << " " << varName;
    }
    objectiveStr << "\n";
    modelFile << objectiveStr.str();
    modelFile << constraintsStr.str();
    modelFile << boundsStr.str();
    modelFile << "end\n";
}

char OptimizationFactorsTablePrinterPass::ID = 0;
static llvm::RegisterPass<OptimizationFactorsTablePrinterPass> X("dump-opt-table","Dump Optimization table");

}

