#pragma once

#include "llvm/Pass.h"
#include <memory>
#include <vector>
#include <unordered_set>

namespace llvm {
class Function;
class Module;
}


namespace vazgen {

class Logger;

class ProgramSlicer
{
public:
    using Slice = std::vector<llvm::Function*>;

public:
    ProgramSlicer(llvm::Module* M, Slice slice, Logger& logger);

    ProgramSlicer(const ProgramSlicer& ) = delete;
    ProgramSlicer(ProgramSlicer&& ) = delete;
    ProgramSlicer& operator =(const ProgramSlicer& ) = delete;
    ProgramSlicer& operator =(ProgramSlicer&& ) = delete;

public:
    bool slice();

    llvm::Module* getSlicedModule()
    {
        return m_slicedModule;
    }

    // TODO: move this to a utility class
    //bool saveModule(llvm::Module* M, const std::string& name);

private:
    llvm::Function* createFunctionDeclaration(llvm::Function* originalF);
    bool changeFunctionUses(llvm::Function* originalF, llvm::Function* cloneF);
    void createSliceModule(const std::unordered_set<std::string>& functions);

private:
    llvm::Module* m_module;
    Slice m_slice;
    llvm::Module* m_slicedModule;
    Logger& m_logger;
}; //class ProgramSlicer

class ProgramSlicerPass : public llvm::ModulePass
{
public:
    static char ID;

    ProgramSlicerPass()
        : llvm::ModulePass(ID)
    {
    }

public:
    void getAnalysisUsage(llvm::AnalysisUsage& AU) const override;
    bool runOnModule(llvm::Module& M) override;

private:
    std::unique_ptr<ProgramSlicer> m_slicer;
}; // class ProgramSlicerPass

} // namespace vazgen

