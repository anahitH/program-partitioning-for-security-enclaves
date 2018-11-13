#pragma once

#include "llvm/Pass.h"
#include <memory>
#include <vector>

namespace llvm {
class Function;
class Module;
}


namespace vazgen {

class ProgramSlicer
{
public:
    using Slice = std::vector<llvm::Function*>;

public:
    ProgramSlicer(llvm::Module* M, Slice slice);

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
    llvm::Module* m_module;
    Slice m_slice;
    llvm::Module* m_slicedModule;
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

