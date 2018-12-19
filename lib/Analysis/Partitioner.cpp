#include "Analysis/Partitioner.h"

#include "Utils/Logger.h"
#include "Utils/Annotation.h"

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
    void computInInterface();
    void computeOutInterface();
    void computeGlobals();

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
        m_partition.addToPartition(m_annotation.getFunction());
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

    void traverseForArgument(llvm::Argument* arg);
    template <typename Container>
    void traverseForward(pdg::FunctionPDG::PDGNodeTy formalArgNode, Container& result);

    template <typename Container>
    void traverseBackward(Container& workingList);

    template <typename Container>
    void collectNodesForActualArg(pdg::PDGLLVMActualArgumentNode& actualArgNode, Container& forwardWorkingList);

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

Partition PartitionForAnnotation::partition()
{
    if (!canPartition()) {
        return m_partition;
    }
    traverse();
    computInInterface();
    computeOutInterface();
    computeGlobals();
    return m_partition;
}

void PartitionForAnnotation::computInInterface()
{
    const auto& partitionFs = m_partition.getPartition();
    Partition::FunctionSet inInterface;
    for (const auto& F : partitionFs) {
        assert(m_pdg->hasFunctionPDG(F));
        const auto& Fpdg = m_pdg->getFunctionPDG(F);
        const auto& callSites = Fpdg->getCallSites();
        for (const auto& callSite : callSites) {
            auto* caller = callSite.getCaller();
            if (partitionFs.find(caller) == partitionFs.end()) {
                inInterface.insert(caller);
                break;
            }
        }
    }
    m_partition.setInInterface(std::move(inInterface));
}

void PartitionForAnnotation::computeOutInterface()
{
    const auto& partitionFs = m_partition.getPartition();
    Partition::FunctionSet outInterface;
    for (const auto& F : partitionFs) {
        assert(m_pdg->hasFunctionPDG(F));
        const auto& Fpdg = m_pdg->getFunctionPDG(F);
        // TODO: think about having call site information embedded in PDG directly.
        for (auto it = Fpdg->llvmNodesBegin(); it != Fpdg->llvmNodesEnd(); ++it) {
            llvm::Value* val = it->first;
            if (!llvm::dyn_cast<llvm::CallInst>(val)
                    && !llvm::dyn_cast<llvm::InvokeInst>(val)) {
                continue;
            }
            for (auto edgeIt = it->second->outEdgesBegin();
                 edgeIt != it->second->outEdgesEnd();
                 ++edgeIt) {
                 if (!(*edgeIt)->isControlEdge()) {
                    continue;
                 }
                 if (auto* functionNode =
                         llvm::dyn_cast<pdg::PDGLLVMFunctionNode>((*edgeIt)->getDestination().get())) {
                    llvm::Function* outF = functionNode->getFunction();
                    if (partitionFs.find(outF) == partitionFs.end()) {
                        outInterface.insert(outF);
                    }
                 }
            }
        }
    }
    m_partition.setOutInterface(outInterface);
}

void PartitionForAnnotation::computeGlobals()
{
    Partition::GlobalsSet referencedGlobals;
    Partition::GlobalsSet modifiedGlobals;
    for (auto glob_it = m_module.global_begin();
         glob_it != m_module.global_end();
         ++glob_it) {
         assert(m_pdg->hasGlobalVariableNode(&*glob_it));
         const auto& globalNode = m_pdg->getGlobalVariableNode(&*glob_it);
         for (auto in_it = globalNode->inEdgesBegin();
              in_it != globalNode->inEdgesEnd();
              ++in_it) {
             referencedGlobals.insert(&*glob_it);
             auto* sourceNode = (*in_it)->getSource().get();
             if (llvm::isa<pdg::PDGPhiNode>(sourceNode)) {
                 modifiedGlobals.insert(&*glob_it);
             } else if (auto* pdgNode = llvm::dyn_cast<pdg::PDGLLVMInstructionNode>(sourceNode)) {
                if (llvm::isa<llvm::StoreInst>(pdgNode->getNodeValue())) {
                    modifiedGlobals.insert(&*glob_it);
                }
             }
         }
         for (auto out_it = globalNode->outEdgesBegin();
              out_it != globalNode->outEdgesEnd();
              ++out_it) {
             referencedGlobals.insert(&*glob_it);
         }
    }
    m_partition.addReferencedGlobals(referencedGlobals);
    m_partition.addModifiedGlobals(modifiedGlobals);
}

bool PartitionForArguments::canPartition() const
{
    if (!m_annotation.hasAnnotatedArguments()) {
        return false;
    }
    llvm::Function* F = m_annotation.getFunction();
    if (!m_pdg->hasFunctionPDG(F)) {
        return false;
    }
    const auto& annotatedArgs = m_annotation.getAnnotatedArguments();
    for (auto arg_idx : annotatedArgs) {
        auto* f_arg = F->arg_begin() + arg_idx;
        if (!f_arg->getType()->isPointerTy()) {
            return false;
        }
    }
    return true;
}

void PartitionForArguments::traverse()
{
    llvm::Function* F = m_annotation.getFunction();
    m_partition.addToPartition(F);
    const auto& annotatedArgs = m_annotation.getAnnotatedArguments();
    for (auto arg_idx : annotatedArgs) {
        auto* f_arg = F->arg_begin() + arg_idx;
        if (f_arg->getType()->isPointerTy()) {
            traverseForArgument(&*f_arg);
        }
    }
}

void PartitionForArguments::traverseForArgument(llvm::Argument* arg)
{
    llvm::Function* F = m_annotation.getFunction();
    auto Fpdg = m_pdg->getFunctionPDG(F);
    if (!Fpdg->hasFormalArgNode(arg)) {
        return;
    }
    std::list<pdg::FunctionPDG::PDGNodeTy> backwardWorkingList;
    auto formalArgNode = Fpdg->getFormalArgNode(arg);
    traverseForward(formalArgNode, backwardWorkingList);

    traverseBackward(backwardWorkingList);
}

template <typename Container>
void PartitionForArguments::traverseForward(pdg::FunctionPDG::PDGNodeTy formalArgNode, Container& result)
{
    auto Fpdg = m_pdg->getFunctionPDG(m_annotation.getFunction());
    std::list<pdg::FunctionPDG::PDGNodeTy> forwardWorkingList;
    std::unordered_set<llvm::Value*> processed_values;
    forwardWorkingList.push_back(formalArgNode);
    while (!forwardWorkingList.empty()) {
        auto* currentNode = forwardWorkingList.back().get();
        forwardWorkingList.pop_back();
        auto* llvmNode = llvm::dyn_cast<pdg::PDGLLVMNode>(currentNode);
        if (!llvmNode) {
            continue;
        }
        auto* nodeValue = llvmNode->getNodeValue();
        if (!nodeValue) {
            continue;
        }
        // TODO: check this
        if (llvm::isa<pdg::PDGLLVMInstructionNode>(llvmNode)) {
            if (!processed_values.insert(nodeValue).second) {
                continue;
            }
        }
        //llvm::dbgs() << "   Node value " << *nodeValue << "\n";
        if (auto* FNode = llvm::dyn_cast<pdg::PDGLLVMFunctionNode>(llvmNode)) {
            m_partition.addToPartition(FNode->getFunction());
            // Stop traversal here
            // TODO: should we stop here or continue for other function calls?
            // i.e. find the formal argument for this call site and continue traversal for it.
            continue;
        }
        if (auto* actualArgNode = llvm::dyn_cast<pdg::PDGLLVMActualArgumentNode>(llvmNode)) {
            collectNodesForActualArg(*actualArgNode, forwardWorkingList);
        }
        if (auto* storeInst = llvm::dyn_cast<llvm::StoreInst>(nodeValue)) {
            auto valueOp = storeInst->getValueOperand();
            if (Fpdg->hasNode(valueOp)) {
                result.push_back(Fpdg->getNode(valueOp));
            }
        }
        for (auto out_it = currentNode->outEdgesBegin();
                out_it != currentNode->outEdgesEnd();
                ++out_it) {
            forwardWorkingList.push_front((*out_it)->getDestination());
        }
    }
}

template <typename Container>
void PartitionForArguments::traverseBackward(Container& workingList)
{
    std::unordered_set<llvm::Value*> processed_values;
    while (!workingList.empty()) {
        auto* currentNode = workingList.back().get();
        workingList.pop_back();
        auto* llvmNode = llvm::dyn_cast<pdg::PDGLLVMNode>(currentNode);
        if (!llvmNode) {
            continue;
        }
        auto* nodeValue = llvmNode->getNodeValue();
        if (!nodeValue) {
            continue;
        }
        // TODO: check this
        if (llvm::isa<pdg::PDGLLVMInstructionNode>(llvmNode)) {
            if (!processed_values.insert(nodeValue).second) {
                continue;
            }
        }
        //llvm::dbgs() << "Node value " << *nodeValue << "\n";
        if (auto* FNode = llvm::dyn_cast<pdg::PDGLLVMFunctionNode>(llvmNode)) {
            if (!FNode->getFunction()->isDeclaration()) {
                m_partition.addToPartition(FNode->getFunction());
            }
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

template <typename Container>
void PartitionForArguments::collectNodesForActualArg(pdg::PDGLLVMActualArgumentNode& actualArgNode,
                                                     Container& forwardWorkingList)
{
    auto* arg = actualArgNode.getCallSite().getArgOperand(actualArgNode.getArgIndex());
    if (!arg->getType()->isPointerTy()) {
        return;
    }
    const int argIdx = actualArgNode.getArgIndex();

    for (auto it = actualArgNode.outEdgesBegin(); it != actualArgNode.outEdgesEnd(); ++it) {
        auto destNode = (*it)->getDestination();
        if (auto* formalArgNode = llvm::dyn_cast<pdg::PDGLLVMFormalArgumentNode>(destNode.get())) {
            llvm::Function* F = formalArgNode->getFunction();
            if (!m_pdg->hasFunctionPDG(F)) {
                continue;
            }
            m_partition.addToPartition(F);
            forwardWorkingList.push_front(destNode);
        }
    }
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
    // TODO: do we need to include new functions in the curse of backward traversal?
    llvm::Function* F = m_annotation.getFunction();
    auto Fpdg = m_pdg->getFunctionPDG(F);
    m_partition.addToPartition(F);

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
            if (!FNode->getFunction()->isDeclaration()) {
                m_partition.addToPartition(FNode->getFunction());
            }
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

Partition Partitioner::partition(const Annotations& annotations)
{
    Partition partition;
    for (const auto& annot : annotations) {
        PartitionForFunction f_partitioner(m_module, annot);
        const auto& f_partition = f_partitioner.partition();
        partition.addToPartition(f_partition);

        PartitionForArguments arg_partitioner(m_module, annot, m_pdg);
        const auto& arg_partition = arg_partitioner.partition();
        partition.addToPartition(arg_partition);

        PartitionForReturnValue ret_partitioner(m_module, annot, m_pdg);
        const auto& ret_partition = ret_partitioner.partition();
        partition.addToPartition(ret_partition);
    }
    return partition;
}

} // namespace vazgen

