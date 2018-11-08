#include "Analysis/Partitioner.h"

#include "Logger.h"
#include "Annotation.h"

#include "PDG/PDG/PDG.h"
#include "PDG/PDG/PDGNode.h"
#include "PDG/PDG/PDGEdge.h"
#include "PDG/PDG/FunctionPDG.h"

#include "llvm/IR/Function.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Constants.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/raw_ostream.h"

#include <list>

namespace vazgen {

namespace {

template <typename Collection>
void collectFunctionReturnNodes(llvm::Function* F, const pdg::FunctionPDG& f_pdg, Collection& returns)
{
    for (auto& B : *F) {
        for (auto& I : B) {
            if (llvm::isa<llvm::ReturnInst>(&I)) {
                if (!f_pdg.hasNode(&I)) {
                    continue;
                }
                returns.push_back(f_pdg.getNode(&I));
            }
        }
    }
}

// This is faster then the one above. But is uses information internal to PDG. Or? is it internal?
template <typename Collection>
void collectFunctionReturnNodes(const pdg::PDGLLVMFunctionNode& Fnode, Collection returns)
{
    for (auto it = Fnode.inEdgesBegin(); it != Fnode.inEdgesEnd(); ++it) {
        auto source = (*it)->getSource();
        if (source->getNodeType() != pdg::PDGLLVMNode::InstructionNode) {
            continue;
        }
        auto* llvmSourceNode = llvm::dyn_cast<pdg::PDGLLVMNode>(source.get());
        assert(llvmSourceNode != nullptr);
        auto* retInst = llvm::dyn_cast<llvm::ReturnInst>(llvmSourceNode->getNodeValue());
        if (!retInst) {
            continue;
        }
        returns.push_back(llvmSourceNode);
    }
}

}

class PartitionForAnnotation
{
public:
    using Partition = Partitioner::Partition;
    using PDGType = std::shared_ptr<pdg::PDG>;

public:
    PartitionForAnnotation(llvm::Module& M, PDGType pdg, const Annotation& annotation)
        : m_module(M)
        , m_annotation(annotation)
        , m_pdg(pdg)
    {
    }


    virtual Partition partition();

protected:
    virtual bool canPartition() const = 0;
    virtual void traverse() = 0;

protected:
    llvm::Module& m_module;
    const Annotation& m_annotation;
    PDGType m_pdg;
    Partition m_partition;
}; // PartitionForAnnotation

/// Implementation of ProgramPartition for annotated function
class PartitionForFunction : public PartitionForAnnotation
{
public:
    PartitionForFunction(llvm::Module& M, const Annotation& annotation)
        : PartitionForAnnotation(M, PDGType(), annotation)
    {
    }

private:
    virtual bool canPartition() const final
    {
        return (m_annotation.getFunction() != nullptr);
    }

    virtual void traverse() final
    {
        m_partition.insert(m_annotation.getFunction());
    }

}; // class PartitionForFunction

/// Implementation of ProgramPartition for annotated function and arguments
class PartitionForArguments : public PartitionForAnnotation
{
public:
    PartitionForArguments(llvm::Module& M,
                          const Annotation& annotation,
                          PDGType pdg)
        : PartitionForAnnotation(M, pdg, annotation)
    {
    }

private:
    virtual bool canPartition() const final;
    virtual void traverse() final;
}; // class PartitionForArguments

/// Implementation of ProgramPartition for annotated function and arguments
class PartitionForReturnValue : public PartitionForAnnotation
{
public:
    PartitionForReturnValue(llvm::Module& M,
                            const Annotation& annotation,
                            PDGType pdg)
        : PartitionForAnnotation(M, pdg, annotation)
    {
    }

private:
    virtual bool canPartition() const final;
    virtual void traverse() final;
}; // class PartitionForArguments

PartitionForAnnotation::Partition PartitionForAnnotation::partition()
{
    if (!canPartition()) {
        return m_partition;
    }
    traverse();
    return m_partition;
}

bool PartitionForArguments::canPartition() const
{
    // TODO:
    return false;
}

void PartitionForArguments::traverse()
{
    // TODO:
}

bool PartitionForReturnValue::canPartition() const
{
    if (!m_annotation.isReturnAnnotated()) {
        return false;
    }
    llvm::Function* F = m_annotation.getFunction();
    if (F->getReturnType()->isVoidTy()) {
        return false;
    }
    if (!m_pdg->hasFunctionPDG(F)) {
        return false;
    }
    return true;
}

void PartitionForReturnValue::traverse()
{
    llvm::Function* F = m_annotation.getFunction();
    auto Fpdg = m_pdg->getFunctionPDG(F);
    m_partition.insert(F);

    std::list<pdg::FunctionPDG::PDGNodeTy> workingList;
    collectFunctionReturnNodes(F, *Fpdg, workingList);
    std::unordered_set<llvm::Value*> processed_values;

    while (!workingList.empty()) {
        auto* currentNode = workingList.back().get();
        workingList.pop_back();
        auto* llvmNode = llvm::dyn_cast<pdg::PDGLLVMNode>(currentNode);
        if (!llvmNode) {
            continue;
        }
        auto* nodeValue = llvmNode->getNodeValue();
        // TODO: check this
        if (llvm::isa<pdg::PDGLLVMInstructionNode>(llvmNode)) {
            if (!processed_values.insert(nodeValue).second) {
                continue;
            }
        }
        //llvm::dbgs() << "Node value " << *nodeValue << "\n";
        if (auto* FNode = llvm::dyn_cast<pdg::PDGLLVMFunctionNode>(llvmNode)) {
            m_partition.insert(FNode->getFunction());
            // Stop traversal here
            continue;
        }
        for (auto in_it = currentNode->inEdgesBegin();
                in_it != currentNode->inEdgesEnd();
                ++in_it) {
            workingList.push_front((*in_it)->getSource());
        }
    }
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

