#include "Analysis/Partitioner.h"

#include "Logger.h"
#include "Annotation.h"

#include "PDG/PDG/PDG.h"

#include "llvm/IR/Function.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Constants.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/raw_ostream.h"


namespace vazgen {

class PartitionForAnnotation
{
public:
    using Partition = Partitioner::Partition;

public:
    PartitionForAnnotation(llvm::Module& M, const Annotation& annotation)
        : m_module(M)
        , m_annotation(annotation)
    {
    }


    virtual Partition partition() = 0;

protected:
    llvm::Module& m_module;
    const Annotation& m_annotation;
}; // PartitionForAnnotation

/// Implementation of ProgramPartition for annotated function
class PartitionForFunction : public PartitionForAnnotation
{
public:
    PartitionForFunction(llvm::Module& M, const Annotation& annotation)
        : PartitionForAnnotation(M, annotation)
    {
    }

    Partition partition() final
    {
        auto* F = m_annotation.getFunction();
        if (!F) {
            return Partition();
        }
        return {F};
    }
}; // class PartitionForFunction

/// Implementation of ProgramPartition for annotated function and arguments
class PartitionForArguments : public PartitionForAnnotation
{
public:
    using PDGType = std::shared_ptr<pdg::PDG>;

public:
    PartitionForArguments(llvm::Module& M,
                          const Annotation& annotation,
                          PDGType pdg)
        : PartitionForAnnotation(M, annotation)
        , m_pdg(pdg)
    {
    }

    Partition partition() final;

private:
    PDGType m_pdg;
}; // class PartitionForArguments

/// Implementation of ProgramPartition for annotated function and arguments
class PartitionForReturnValue : public PartitionForAnnotation
{
public:
    using PDGType = std::shared_ptr<pdg::PDG>;

public:
    PartitionForReturnValue(llvm::Module& M,
                            const Annotation& annotation,
                            PDGType pdg)
        : PartitionForAnnotation(M, annotation)
        , m_pdg(pdg)
    {
    }

    Partition partition() final;

private:
    PDGType m_pdg;
}; // class PartitionForArguments

PartitionForAnnotation::Partition PartitionForArguments::partition()
{
    // TODO:
    return Partition();
}

PartitionForAnnotation::Partition PartitionForReturnValue::partition()
{
    // TODO:
    return Partition();
}

Partitioner::Partition Partitioner::partition(const Annotations& annotations)
{
    Partition partition;
    for (const auto& annot : annotations) {
        PartitionForFunction f_partitioner(m_module, annot);
        const auto& f_partition = f_partitioner.partition();
        partition.insert(f_partition.begin(), f_partition.end());

        PartitionForArguments arg_partitioner(m_module, annot, m_pdg);
        const auto& arg_partition = arg_partitioner.partition();
        partition.insert(arg_partition.begin(), arg_partition.end());

        PartitionForReturnValue ret_partitioner(m_module, annot, m_pdg);
        const auto& ret_partition = ret_partitioner.partition();
        partition.insert(ret_partition.begin(), ret_partition.end());
    }
    return partition;
}

} // namespace vazgen

