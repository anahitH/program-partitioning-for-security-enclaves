#pragma once

#include "Utils/Annotation.h"
#include "Partition.h"

#include "PDG/PDG/FunctionPDG.h"

#include <memory>
#include <vector>
#include <unordered_set>

namespace llvm {
class Module;
}

namespace pdg {
class PDG;
class PDGLLVMActualArgumentNode;
}

namespace vazgen {

class Logger;

class PartitionForAnnotation
{
public:
    using PDGType = std::shared_ptr<pdg::PDG>;

public:
    PartitionForAnnotation(llvm::Module& M,
                           PDGType pdg,
                           const Annotation& annotation,
                           Logger& logger);
protected:
    virtual bool canPartition() const = 0;
    virtual void traverse() = 0;

protected:
    llvm::Module& m_module;
    const Annotation& m_annotation;
    PDGType m_pdg;
    Partition m_partition;
    Logger& m_logger;
}; // PartitionForAnnotation

class FunctionLevelPartitionForAnnotation : public PartitionForAnnotation
{
public:
    FunctionLevelPartitionForAnnotation(llvm::Module& M,
                                        PDGType pdg,
                                        const Annotation& annotation,
                                        Logger& logger);

    virtual Partition partition();

protected:
    Partition m_partition;
};

/// Implementation of ProgramPartition for annotated function
class PartitionForFunction : public FunctionLevelPartitionForAnnotation
{
public:
    PartitionForFunction(llvm::Module& M,
                         const Annotation& annotation,
                         Logger& logger);

private:
    virtual bool canPartition() const final
    {
        return (m_annotation.getFunction() != nullptr);
    }

    virtual void traverse() final;
}; // class PartitionForFunction

/// Implementation of ProgramPartition for annotated function and arguments
class PartitionForArguments : public FunctionLevelPartitionForAnnotation
{
public:
    PartitionForArguments(llvm::Module& M,
                          const Annotation& annotation,
                          PDGType pdg,
                          Logger& logger);

private:
    virtual bool canPartition() const final;
    virtual void traverse() final;

    void traverseForArgument(llvm::Argument* arg);
    template <typename Container>
    void traverseForward(pdg::FunctionPDG::PDGNodeTy formalArgNode, Container& result);

    template <typename Container>
    void traverseBackward(Container& workingList);

    template <typename Container>
    void collectNodesForActualArg(pdg::PDGLLVMActualArgumentNode& actualArgNode, Container& forwardWorkingList);
}; // class PartitionForArguments

/// Implementation of ProgramPartition for annotated function and arguments
class PartitionForReturnValue : public FunctionLevelPartitionForAnnotation
{
public:
    PartitionForReturnValue(llvm::Module& M,
                            const Annotation& annotation,
                            PDGType pdg,
                            Logger& logger);
private:
    virtual bool canPartition() const final;
    virtual void traverse() final;
}; // class PartitionForArguments

class PartitionGlobals
{
public:
    using PDGType = std::shared_ptr<pdg::PDG>;

    PartitionGlobals(llvm::Module& module,
                     PDGType pdg,
                     const Partition& partition,
                     Logger& logger);
public:
    void partition();

    const Partition::GlobalsSet& getReferencedGlobals() const
    {
        return m_referencedGlobals;
    }

private:
    llvm::Module& m_module;
    PDGType m_pdg;
    const Partition& m_partition;
    Logger& m_logger;
    Partition::GlobalsSet m_referencedGlobals;
}; // class PartitionGlobals


} // namespace vazgen

