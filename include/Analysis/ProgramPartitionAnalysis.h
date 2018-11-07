#pragma once

#include "llvm/Pass.h"
#include "PDG/PDG/PDG.h"

#include <memory>
#include <vector>
#include <unordered_set>

namespace llvm {
class Module;
class Function;
}

namespace vazgen {

class Annotation;
/**
 * \class ProgramPartition
 * \brief Partitions given Module based on user annotations
 */
class ProgramPartition
{
public:
    // TODO: what is partition does not only include functions but for example also global variables?
    using Partition = std::unordered_set<llvm::Function*>;
    using Annotations = std::vector<Annotation>;
    using PDGType = std::shared_ptr<pdg::PDG>;

public:
    ProgramPartition(llvm::Module& M, PDGType pdg);

    ProgramPartition(const ProgramPartition& ) = delete;
    ProgramPartition(ProgramPartition&& ) = delete;
    ProgramPartition& operator =(const ProgramPartition& ) = delete;
    ProgramPartition& operator =(ProgramPartition&& ) = delete;


public:
    void partition(const Annotations& annotations);

    const Partition& getPartition() const
    {
        return m_partition;
    }

    Partition& getPartition()
    {
        return m_partition;
    }

public:
    void dump() const;

private:
    llvm::Module& m_module;
    PDGType m_pdg;
    Partition m_partition;
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

private:
    std::unique_ptr<ProgramPartition> m_partition;
}; // class ProgramPartitionAnalysis

} // namespace vazgen

