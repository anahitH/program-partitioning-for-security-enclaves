#pragma once

#include <unordered_map>
#include <functional>
#include <vector>
#include <cassert>

namespace llvm {
class BasicBlock;
class Function;
class LoopInfo;
class Loop;
class Module;
}

namespace pdg {
class PDG;
class FunctionPDG;
}

namespace debug {

class ILPBlockOptimizationDataRow
{
public:
    using ContextSwitchData = std::unordered_map<llvm::Function*, int>;

public:
    ILPBlockOptimizationDataRow(llvm::BasicBlock* B,
                                pdg::FunctionPDG* pdg,
                                llvm::Loop* loop);

    llvm::BasicBlock* getBlock() const
    {
        return m_block;
    }

    llvm::Function* getFunction() const;
    int getBSize() const;
    int getCallNum(llvm::Function* F) const;
    void fillRow();
    std::string getRowAsString() const;

    const ContextSwitchData& getContextSwitchData() const
    {
        return m_contextSwitchNum;
    }

private:
    llvm::BasicBlock* m_block;
    llvm::Loop* m_loop;
    pdg::FunctionPDG* m_fPdg;
    ContextSwitchData m_contextSwitchNum;
}; // class ILPOptimizationDataRow

class ILPBlockOptimizationData
{
public:
    using LoopInfoGetter = std::function<llvm::LoopInfo* (llvm::Function*)>;
    using BlockOptData =  std::unordered_map<llvm::BasicBlock*, ILPBlockOptimizationDataRow>;

public:
    ILPBlockOptimizationData(llvm::Module& M,
                             pdg::PDG* pdg,
                             const LoopInfoGetter& loopInfoGetter);

    void collectOptimizationData();

    const BlockOptData&
    getILPBlockOptimizationData() const
    {
        return m_ilpOptData;
    }

    const ILPBlockOptimizationDataRow&
    getILPBlockOptimizationData(llvm::BasicBlock* B) const
    {
        auto pos = m_ilpOptData.find(B);
        assert(pos != m_ilpOptData.end());
        return pos->second;
    }

    const std::unordered_map<llvm::Function*, int> getFunctions() const
    {
        return m_functions;
    }

private:
    llvm::Module& m_M;
    pdg::PDG* m_pdg;
    const LoopInfoGetter& m_loopInfoGetter;
    BlockOptData m_ilpOptData;
    std::unordered_map<llvm::Function*, int> m_functions;
};


}

