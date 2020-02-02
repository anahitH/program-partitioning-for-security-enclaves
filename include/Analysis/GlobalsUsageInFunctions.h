#pragma once

#include <memory>
#include <unordered_map>
#include <vector>

namespace llvm {
class Module;
class Function;
class GlobalVariable;
}

namespace pdg {
class PDG;
}

namespace vazgen {

class Logger;
class GlobalsUsageInFunctions
{
public:
    using GlobalVariables = std::vector<llvm::GlobalVariable*>;
    using GlobalsInFunctions = std::unordered_map<llvm::Function*, GlobalVariables>;
    using PDGType = std::shared_ptr<pdg::PDG>;

public:
    static GlobalsUsageInFunctions& getGlobalsUsageInFunctions()
    {
        static GlobalsUsageInFunctions globalsUsageInFunctions;
        return globalsUsageInFunctions;
    }

    static void computeGlobalsUsageInFunctions(llvm::Module& module,
                                               PDGType pdg,
                                               Logger& logger);

public:
    bool functionIsUsingGlobals(llvm::Function* F) const;
    const GlobalVariables& getGlobalVariablesUsedInFunction(llvm::Function* F) const;

    void setGlobalsUsedInFunctions(const GlobalsInFunctions& globalsInFunctions);
    void addGlobalsUsedInFunction(llvm::Function* F, const GlobalVariables& globals);
    void addGlobalUsedInFunction(llvm::Function* F, llvm::GlobalVariable* global);
    

private:
    GlobalsInFunctions m_globalsUsedInFunctions; 
}; // class GlobalUsageInFunctions

}; // namespace vazgen

