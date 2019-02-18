#include "Analysis/Partitioner.h"

#include "Utils/PartitionUtils.h"
#include "Utils/Annotation.h"
#include "Utils/Logger.h"
#include "Utils/Utils.h"

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
    PartitionForAnnotation(llvm::Module& M,
                           PDGType pdg,
                           const Annotation& annotation,
                           Logger& logger)
        : m_module(M)
        , m_annotation(annotation)
        , m_pdg(pdg)
        , m_logger(logger)
    {
    }


    virtual Partition partition();

protected:
    virtual bool canPartition() const = 0;
    virtual void traverse() = 0;

    int getFunctionLevel(pdg::PDGLLVMNode* node);
    int getFunctionLevel(llvm::Function* F);

protected:
    llvm::Module& m_module;
    const Annotation& m_annotation;
    PDGType m_pdg;
    Partition m_partition;
    Logger& m_logger;
}; // PartitionForAnnotation

/// Implementation of ProgramPartition for annotated function
class PartitionForFunction : public PartitionForAnnotation
{
public:
    PartitionForFunction(llvm::Module& M, const Annotation& annotation, Logger& logger)
        : PartitionForAnnotation(M, PDGType(), annotation, logger)
    {
    }

private:
    virtual bool canPartition() const final
    {
        return (m_annotation.getFunction() != nullptr);
    }

    virtual void traverse() final
    {
        m_logger.info("Partitioning for sensitive functions");
        m_partition.addToPartition(m_annotation.getFunction());
    }

}; // class PartitionForFunction

/// Implementation of ProgramPartition for annotated function and arguments
class PartitionForArguments : public PartitionForAnnotation
{
public:
    PartitionForArguments(llvm::Module& M,
                          const Annotation& annotation,
                          PDGType pdg,
                          Logger& logger)
        : PartitionForAnnotation(M, pdg, annotation, logger)
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
                            PDGType pdg,
                            Logger& logger)
        : PartitionForAnnotation(M, pdg, annotation, logger)
    {
    }

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
                     Logger& logger)
        : m_module(module)
        , m_pdg(pdg)
        , m_partition(partition)
        , m_logger(logger)
    {
    }

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

Partition PartitionForAnnotation::partition()
{
    if (!canPartition()) {
        return m_partition;
    }
    traverse();
    return m_partition;
}

int PartitionForAnnotation::getFunctionLevel(pdg::PDGLLVMNode* node)
{
    if (!node) {
        return -1;
    }
    if (!node->getNodeValue()) {
        return -1;
    }
    llvm::Function* F = nullptr;

    if (auto* instr = llvm::dyn_cast<llvm::Instruction>(node->getNodeValue())) {
        F = instr->getFunction();
    } else if (auto* block = llvm::dyn_cast<llvm::BasicBlock>(node->getNodeValue())) {
        F = block->getParent();
    }
    if (!F) {
        return -1;
    }
    if (m_partition.contains(F)) {
        return 0;
    }
    int level = m_partition.getFunctionRelationLevel(F);
    if (level < 0) {
        m_logger.error("Function " + F->getName().str() + " is not added to partition related functions");
        return level;
    }
    return level;
}

int PartitionForAnnotation::getFunctionLevel(llvm::Function* F)
{
    if (m_partition.contains(F)) {
        return 0;
    }
    int level = m_partition.getFunctionRelationLevel(F);
    if (level < 0) {
        m_logger.error("Function " + F->getName().str() + " is not added to partition related functions");
    }
    return level;
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
    m_logger.info("Partitioning for sensitive arguments");
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
    pdg::PDGNode* prevNode = nullptr;
    pdg::PDGNode* currentNode = nullptr;
    while (!forwardWorkingList.empty()) {
        prevNode = currentNode;
        currentNode = forwardWorkingList.back().get();
        forwardWorkingList.pop_back();
        auto* llvmNode = llvm::dyn_cast<pdg::PDGLLVMNode>(currentNode);
        if (!llvmNode) {
            continue;
        }
        auto* nodeValue = llvmNode->getNodeValue();
        if (!nodeValue) {
            continue;
        }
        //llvm::dbgs() << "   Node value " << *nodeValue << "\n";
        // TODO: check this
        if (llvm::isa<pdg::PDGLLVMInstructionNode>(llvmNode)) {
            if (!processed_values.insert(nodeValue).second) {
                continue;
            }
        }
        if (auto* FNode = llvm::dyn_cast<pdg::PDGLLVMFunctionNode>(llvmNode)) {
            auto* F = FNode->getFunction(); 
            if (!F->isDeclaration()) {
                if (F == m_annotation.getFunction()) {
                    continue;
                }
                int level = prevNode ? getFunctionLevel(llvm::dyn_cast<pdg::PDGLLVMNode>(prevNode)) : 0;
                if (level >= 0) {
                    m_partition.addRelatedFunction(F, level + 1);
                } else {
                    m_logger.error("Negative level for " + F->getName().str() + ". Does not add to partition related functions");
                }

            }
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
    pdg::PDGNode* prevNode = nullptr;
    pdg::PDGNode* currentNode = nullptr;
    while (!workingList.empty()) {
        prevNode = currentNode;
        currentNode = workingList.back().get();
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
            auto* F = FNode->getFunction(); 
            if (F == m_annotation.getFunction()) {
                continue;
            }
            if (!F->isDeclaration()) {
                int level = prevNode ? getFunctionLevel(llvm::dyn_cast<pdg::PDGLLVMNode>(prevNode)) : 0;
                if (level >= 0) {
                    m_partition.addRelatedFunction(F, level + 1);
                } else {
                    m_logger.error("Negative level for " + F->getName().str() + ". Does not add to partition related functions");
                }

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
            if (!F->isDeclaration()) {
                int level = getFunctionLevel(actualArgNode.getCallSite().getCaller());
                if (level >= 0) {
                    m_partition.addRelatedFunction(F, level + 1);
                } else {
                    m_logger.error("Negative level for " + F->getName().str() + ". Does not add to partition related functions");
                }

            }
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
    m_logger.info("Partitioning for sensitive return values");
    // TODO: do we need to include new functions in the curse of backward traversal?
    llvm::Function* F = m_annotation.getFunction();
    auto Fpdg = m_pdg->getFunctionPDG(F);
    m_partition.addToPartition(F);

    std::list<pdg::FunctionPDG::PDGNodeTy> workingList;
    collectFunctionReturnNodes(F, *Fpdg, workingList);
    std::unordered_set<llvm::Value*> processed_values;

    pdg::PDGNode* prevNode = nullptr;
    pdg::PDGNode* currentNode = nullptr;
    while (!workingList.empty()) {
        prevNode = currentNode;
        currentNode = workingList.back().get();
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
            llvm::Function* F = FNode->getFunction();
            if (F == m_annotation.getFunction()) {
                continue;
            }
            if (!F->isDeclaration()) {
                int level = prevNode ? getFunctionLevel(llvm::dyn_cast<pdg::PDGLLVMNode>(prevNode)) : 0;
                if (level >= 0) {
                    m_partition.addRelatedFunction(F, level + 1);
                } else {
                    m_logger.error("Negative level for " + F->getName().str() + ". Does not add to partition related functions");
                }
            }
            continue;
        }
        for (auto in_it = currentNode->inEdgesBegin();
                in_it != currentNode->inEdgesEnd();
                ++in_it) {
            workingList.push_front((*in_it)->getSource());
        }
    }
}

void PartitionGlobals::partition()
{
    // TODO: this probably is not an accurate message.
    m_logger.info("Partitioning globals");
    for (auto glob_it = m_module.global_begin();
         glob_it != m_module.global_end();
         ++glob_it) {
         assert(m_pdg->hasGlobalVariableNode(&*glob_it));
         const auto& globalNode = m_pdg->getGlobalVariableNode(&*glob_it);
         for (auto in_it = globalNode->inEdgesBegin();
              in_it != globalNode->inEdgesEnd();
              ++in_it) {
             if (m_partition.contains((*in_it)->getSource()->getParent())) {
                 m_referencedGlobals.insert(&*glob_it);
             }
         }
         for (auto out_it = globalNode->outEdgesBegin();
              out_it != globalNode->outEdgesEnd();
              ++out_it) {
             if (m_partition.contains((*out_it)->getDestination()->getParent())) {
                 m_referencedGlobals.insert(&*glob_it);
             }
         }
    }
}

void Partitioner::computeInsecurePartition()
{
    for (auto& F : m_module) {
        if (!m_securePartition.contains(&F)) {
            m_insecurePartition.addToPartition(&F);
        }
    }
    PartitionGlobals globals_partitioner(m_module, m_pdg, m_insecurePartition, m_logger);
    globals_partitioner.partition();
    m_insecurePartition.setGlobals(globals_partitioner.getReferencedGlobals());

    m_insecurePartition.setInInterface(PartitionUtils::computeInInterface(m_insecurePartition.getPartition(), *m_pdg));
    m_insecurePartition.setOutInterface(PartitionUtils::computeOutInterface(m_insecurePartition.getPartition(), *m_pdg));
}

void Partitioner::partition(const Annotations& annotations)
{
    for (const auto& annot : annotations) {
        PartitionForFunction f_partitioner(m_module, annot, m_logger);
        const auto& f_partition = f_partitioner.partition();
        m_securePartition.addToPartition(f_partition);

        PartitionForArguments arg_partitioner(m_module, annot, m_pdg, m_logger);
        const auto& arg_partition = arg_partitioner.partition();
        m_securePartition.addToPartition(arg_partition);

        PartitionForReturnValue ret_partitioner(m_module, annot, m_pdg, m_logger);
        const auto& ret_partition = ret_partitioner.partition();
        m_securePartition.addToPartition(ret_partition);
    }
    PartitionGlobals globals_partitioner(m_module, m_pdg, m_securePartition, m_logger);
    globals_partitioner.partition();
    m_securePartition.setGlobals(globals_partitioner.getReferencedGlobals());

    m_securePartition.setInInterface(PartitionUtils::computeInInterface(m_securePartition.getPartition(), *m_pdg));
    m_securePartition.setOutInterface(PartitionUtils::computeOutInterface(m_securePartition.getPartition(), *m_pdg));
    computeInsecurePartition();
}

} // namespace vazgen

