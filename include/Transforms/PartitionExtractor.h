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
class Partition;

class PartitionExtractor
{
public:
    PartitionExtractor(llvm::Module* M,
                       const Partition& partition,
                       Logger& logger);

    PartitionExtractor(const PartitionExtractor& ) = delete;
    PartitionExtractor(PartitionExtractor&& ) = delete;
    PartitionExtractor& operator =(const PartitionExtractor& ) = delete;
    PartitionExtractor& operator =(PartitionExtractor&& ) = delete;

public:
    bool extract();

    const std::unique_ptr<llvm::Module>& getSlicedModule()
    {
        return m_slicedModule;
    }

private:
    llvm::Function* createFunctionDeclaration(llvm::Function* originalF);
    bool changeFunctionUses(llvm::Function* originalF, llvm::Function* cloneF);
    void createModule(const std::unordered_set<std::string>& functions);

private:
    llvm::Module* m_module;
    const Partition& m_partition;
    std::unique_ptr<llvm::Module> m_slicedModule;
    Logger& m_logger;
}; //class PartitionExtractor

class PartitionExtractorPass : public llvm::ModulePass
{
public:
    static char ID;

    PartitionExtractorPass()
        : llvm::ModulePass(ID)
    {
    }

public:
    void getAnalysisUsage(llvm::AnalysisUsage& AU) const override;
    bool runOnModule(llvm::Module& M) override;

private:
    bool sliceForPartition(Logger& logger, llvm::Module& M, bool enclave);

private:
    std::unique_ptr<PartitionExtractor> m_extractor;
}; // class PartitionExtractorPass

} // namespace vazgen

