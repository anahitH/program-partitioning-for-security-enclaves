#pragma once

#include <unordered_map>
#include <functional>
#include <vector>

namespace llvm {
class Function;
class LoopInfo;
class Module;
}

namespace pdg {
class PDG;
}

namespace debug {

class ILPOptimizationDataRow
{
public:
    using ContextSwitchData = std::unordered_map<llvm::Function*, int>;

public:
    ILPOptimizationDataRow(llvm::Function* F,
                           pdg::PDG* pdg,
                           llvm::LoopInfo* loopInfo);

    llvm::Function* getFunction() const
    {
        return m_F;
    }

    int getFSize() const
    {
        return m_Fsize;
    }

    int getCallNum(llvm::Function* F) const;
    void fillRow();
    std::string getRowAsString() const;

    const ContextSwitchData& getContextSwitchData() const
    {
        return m_contextSwitchNum;
    }

private:
    llvm::Function* m_F;
    llvm::LoopInfo* m_loopInfo;
    pdg::PDG* m_pdg;
    int m_Fsize;
    ContextSwitchData m_contextSwitchNum;
}; // class ILPOptimizationDataRow

class ILPOptimizationData
{
public:
    using LoopInfoGetter = std::function<llvm::LoopInfo* (llvm::Function*)>;

public:
    ILPOptimizationData(llvm::Module& M,
                        pdg::PDG* pdg,
                        const LoopInfoGetter& loopInfoGetter);

    void collectOptimizationData();

    const std::vector<ILPOptimizationDataRow>&
    getILPOptimizationData() const
    {
        return m_ilpOptData;
    }

    const std::unordered_map<llvm::Function*, int> getFunctions() const
    {
        return m_functions;
    }

private:
    llvm::Module& m_M;
    pdg::PDG* m_pdg;
    const LoopInfoGetter& m_loopInfoGetter;
    std::vector<ILPOptimizationDataRow> m_ilpOptData;
    std::unordered_map<llvm::Function*, int> m_functions;
};

} // namespace debug

