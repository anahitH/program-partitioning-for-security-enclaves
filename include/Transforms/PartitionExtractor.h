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
    using FunctionSet = std::unordered_set<llvm::Function*>;

public:
    PartitionExtractor(llvm::Module* M,
                       const Partition& partition,
                       const FunctionSet& additionalFunctions,
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
    const FunctionSet& m_additionalFunctions;
    std::unique_ptr<llvm::Module> m_slicedModule;
    Logger& m_logger;
}; //class PartitionExtractor

class PartitionExtractorPass : public llvm::ModulePass
{
public:
    using FunctionSet = std::unordered_set<llvm::Function*>;

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
    FunctionSet getGlobalSetters(llvm::Module& M, Logger& logger, bool isEnclave);
    bool extractPartition(Logger& logger, llvm::Module& M, const FunctionSet& globalSetters, bool enclave);
    void renameInsecureCalls(const std::string& prefix, llvm::Module* M, const Partition& insecurePartition);

private:
    std::unique_ptr<PartitionExtractor> m_extractor;
}; // class PartitionExtractorPass

} // namespace vazgen

