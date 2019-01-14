#include "Analysis/PartitionForAnnotation.h"

#include "Utils/PartitionUtils.h"
#include "Utils/Annotation.h"
#include "Utils/Logger.h"
#include "Utils/Utils.h"

#include "PDG/PDG/PDG.h"
#include "PDG/PDG/PDGNode.h"
#include "PDG/PDG/PDGEdge.h"

#include "llvm/IR/Function.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Constants.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/raw_ostream.h"

#include <list>

namespace vazgen {

PartitionForAnnotation::PartitionForAnnotation(llvm::Module& M,
                                               PDGType pdg,
                                               const Annotation& annotation,
                                               Logger& logger)
    : m_module(M)
    , m_annotation(annotation)
    , m_pdg(pdg)
    , m_logger(logger)
{
}

Partition PartitionForAnnotation::partition()
{
    if (!canPartition()) {
        return m_partition;
    }
    traverse();
    return m_partition;
}

PartitionForFunction::PartitionForFunction(llvm::Module& M,
                                           const Annotation& annotation,
                                           Logger& logger)
    : PartitionForAnnotation(M, PDGType(), annotation, logger)
{
}

void PartitionForFunction::traverse()
{
    m_logger.info("Partitioning for sensitive functions");
    m_partition.addToPartition(m_annotation.getFunction());
}

PartitionForArguments::PartitionForArguments(llvm::Module& M,
                                             const Annotation& annotation,
                                             PDGType pdg,
                                             Logger& logger)
    : PartitionForAnnotation(M, pdg, annotation, logger)
{
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
            if (!FNode->getFunction()->isDeclaration()) {
                m_partition.addToPartition(FNode->getFunction());
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
            if (!F->isDeclaration()) {
                m_partition.addToPartition(F);
            }
            forwardWorkingList.push_front(destNode);
        }
    }
}

PartitionForReturnValue::PartitionForReturnValue(llvm::Module& M,
                                                 const Annotation& annotation,
                                                 PDGType pdg,
                                                 Logger& logger)
    : PartitionForAnnotation(M, pdg, annotation, logger)
{
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

PartitionGlobals::PartitionGlobals(llvm::Module& module,
                                   PDGType pdg,
                                   const Partition& partition,
                                   Logger& logger)
    : m_module(module)
    , m_pdg(pdg)
    , m_partition(partition)
    , m_logger(logger)
{
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
             if (!m_partition.contains(Utils::getNodeParent((*in_it)->getSource().get()))) {
                 continue;
             }
             m_referencedGlobals.insert(&*glob_it);
         }
         for (auto out_it = globalNode->outEdgesBegin();
              out_it != globalNode->outEdgesEnd();
              ++out_it) {
             if (!m_partition.contains(Utils::getNodeParent((*out_it)->getDestination().get()))) {
                 continue;
             }
             m_referencedGlobals.insert(&*glob_it);
         }
    }
}

} // namespace vazgen

