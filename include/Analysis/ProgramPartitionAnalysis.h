#pragma once

#include "Partition.h"
#include "CallGraph.h"

#include "llvm/Pass.h"
#include "PDG/PDG/PDG.h"

#include <memory>
#include <vector>
#include <unordered_set>

namespace llvm {
class Module;
class Function;
class LoopInfo;
class CallGraph;
}

namespace vazgen {

class Annotation;
class Logger;

/**
 * \class ProgramPartition
 * \brief Partitions given Module based on user annotations
 */
class ProgramPartition
{
public:
    // TODO: what is partition does not only include functions but for example also global variables?
    using Annotations = std::vector<Annotation>;
    using PDGType = std::shared_ptr<pdg::PDG>;
    using LoopInfoGetter = std::function<llvm::LoopInfo* (llvm::Function*)>;

public:
    ProgramPartition(llvm::Module& M, PDGType pdg,
                     const llvm::CallGraph& callGraph,
                     const LoopInfoGetter& loopInfoGetter,
                     Logger& logger);

    ProgramPartition(const ProgramPartition& ) = delete;
    ProgramPartition(ProgramPartition&& ) = delete;
    ProgramPartition& operator =(const ProgramPartition& ) = delete;
    ProgramPartition& operator =(ProgramPartition&& ) = delete;


public:
    void partition(const Annotations& annotations);
    void optimize(auto optimizations);

    const Partition& getSecurePartition() const
    {
        return m_securePartition;
    }

    Partition& getSecurePartition()
    {
        return m_securePartition;
    }

    const Partition& getInsecurePartition() const
    {
        return m_securePartition;
    }

    Partition& getInsecurePartition()
    {
        return m_insecurePartition;
    }

public:
    void dump(const std::string& outFile = std::string()) const;
    void dumpStats(const std::string& statsFile = std::string()) const;

private:
    llvm::Module& m_module;
    PDGType m_pdg;
    CallGraph m_callgraph;
    const LoopInfoGetter& m_loopInfoGetter;
    Logger& m_logger;
    Partition m_securePartition;
    Partition m_insecurePartition;
}; // class ProgramPartition

class ProgramPartitionAnalysis : public llvm::ModulePass
{
public:
    static char ID;

    ProgramPartitionAnalysis()
        : llvm::ModulePass(ID)
    {
    }

public:
    void getAnalysisUsage(llvm::AnalysisUsage& AU) const override;
    bool runOnModule(llvm::Module& M) override;

public:
    const ProgramPartition& getProgramPartition() const
    {
        return *m_partition.get();
    }

    ProgramPartition& getProgramPartition()
    {
        return *m_partition.get();
    }

private:
    std::unique_ptr<ProgramPartition> m_partition;
}; // class ProgramPartitionAnalysis

} // namespace vazgen

