#include "Analysis/CallGraph.h"

#include "Utils/Utils.h"
#include "PDG/PDG/PDG.h"
#include "PDG/PDG/FunctionPDG.h"

#include "llvm/IR/Type.h"
#include "llvm/IR/Function.h"
#include "llvm/Analysis/CallGraph.h"
#include "llvm/Analysis/LoopInfo.h"

#include <limits>

namespace vazgen {

namespace {

using CallSiteInfo = std::unordered_map<llvm::Function*, bool>;
// For each function functions it is calling with being in loop information
using FunctionCallSiteInfo = std::unordered_map<llvm::Function*, CallSiteInfo>;

FunctionCallSiteInfo
getFunctionsCallSiteInfo(const pdg::PDG* pdg, const CallGraph::LoopInfoGetter& loopInfoGetter)
{
    FunctionCallSiteInfo functionsCallSiteInfo;
    for (auto& node : pdg->getFunctionPDGs()) {
        llvm::Function* callee = node.first;
        for (auto& callSite : node.second->getCallSites()) {
            llvm::Function* caller = callSite.getCaller();
            auto* loopInfo = loopInfoGetter(caller);
            auto* callerBlock = callSite.getParent();
            const bool isCallSiteInLoop = (loopInfo->getLoopFor(callerBlock) != nullptr);
            functionsCallSiteInfo[caller].insert(std::make_pair(callee, isCallSiteInLoop));
        }
    }
    return functionsCallSiteInfo;
}

std::unordered_map<const llvm::CallSite*, bool>
getCallSiteLoopInformation(const pdg::PDG* pdg,
                           const CallGraph::LoopInfoGetter& loopInfoGetter)
{
    std::unordered_map<const llvm::CallSite*, bool> callSiteInLoop;
    for (auto& node : pdg->getFunctionPDGs()) {
        auto Fpdg = node.second;
        const auto& callSites = Fpdg->getCallSites();
        for (auto& callSite : callSites) {
            llvm::Function* caller = callSite.getCaller();
            auto* loopInfo = loopInfoGetter(caller);
            auto* callerBlock = callSite.getParent();
            callSiteInLoop[&callSite]
                = (loopInfo->getLoopFor(callerBlock) != nullptr);
        }
    }
    return callSiteInLoop;
}

int getTypeComplexity(llvm::Type* type)
{
    int complexity = 0;
    if (auto* structType = llvm::dyn_cast<llvm::StructType>(type)) {
        for (auto it = structType->element_begin(); it != structType->element_end(); ++it) {
            complexity += getTypeComplexity(*it);
        }
    } else if (auto* arrayType = llvm::dyn_cast<llvm::ArrayType>(type)) {
        complexity += arrayType->getNumElements() * getTypeComplexity(arrayType->getElementType());
    } else if (auto* vectorType = llvm::dyn_cast<llvm::VectorType>(type)) {
        complexity += vectorType->getNumElements() * getTypeComplexity(vectorType->getElementType());
    } else if (auto* ptrType = llvm::dyn_cast<llvm::PointerType>(type)) {
        // TODO: think about the cost for this
    } else if (auto* functionType = llvm::dyn_cast<llvm::FunctionType>(type)) {
        // TODO: think about the cost for this, probably should be very high
    } else {
        complexity += 1;
    }
    return complexity;
}

int getArgComplexity(llvm::Function* F)
{
    int complexity = 0;
    for (auto it = F->arg_begin(); it != F->arg_end(); ++it) {
        complexity += getTypeComplexity(it->getType());
    }
    return complexity;
}

std::string getNodeFactorName(WeightFactor::Factor fact)
{
    switch (fact) {
    case WeightFactor::SENSITIVE:
        return "sensitive";
    case WeightFactor::SIZE:
        return "size";
    case WeightFactor::CALL_NUM:
        return "in_loop";
    case WeightFactor::ARG_NUM:
        return "arg_num";
    case WeightFactor::ARG_COMPLEXITY:
        return "arg_complexity";
    case WeightFactor::RET_COMPLEXITY:
        return "ret_complexity";
    default:
        assert(false);
    };
    return "";
}
} // unnamed namespace

class WeightAssigningHelper
{
public:
    using LoopInfoGetter = CallGraph::LoopInfoGetter;
    using CallSiteData = std::unordered_map<llvm::Function*, std::unordered_map<llvm::Function*, int>>;

public:
    WeightAssigningHelper(CallGraph& callGraph,
                          const Partition& securePartition,
                          const Partition& insecurePartition,
                          const pdg::PDG* pdg,
                          const LoopInfoGetter& loopInfoGetter);
    
    void assignWeights();

private:
    void assignNodeWeights();
    void assignSensitiveNodeWeights();
    void assignNodeSizeWeights();
    void assignEdgeWeights();
    void assignCallNumWeights();
    void assignArgWeights();
    void assignRetValueWeights();
    CallSiteData collectFunctionCallSiteData();

private:
    CallGraph& m_callGraph;
    const Partition& m_securePartition;
    const Partition& m_insecurePartition;
    const pdg::PDG* m_pdg;
    const LoopInfoGetter& m_loopInfoGetter;
}; // class WeightAssigningHelper

WeightAssigningHelper::WeightAssigningHelper(CallGraph& callGraph,
                                             const Partition& securePartition,
                                             const Partition& insecurePartition,
                                             const pdg::PDG* pdg,
                                             const LoopInfoGetter& loopInfoGetter)
    : m_callGraph(callGraph)
    , m_securePartition(securePartition)
    , m_insecurePartition(insecurePartition)
    , m_pdg(pdg)
    , m_loopInfoGetter(loopInfoGetter)
{
}
 
void WeightAssigningHelper::assignWeights()
{
    assignNodeWeights();
    assignEdgeWeights();
}

void WeightAssigningHelper::assignNodeWeights()
{
    assignSensitiveNodeWeights();
    assignNodeSizeWeights();
}

void WeightAssigningHelper::assignSensitiveNodeWeights()
{
    WeightFactor factor(WeightFactor::SENSITIVE);
    factor.setValue(1);
    for (llvm::Function* F : m_securePartition.getPartition()) {
        auto* Fnode = m_callGraph.getFunctionNode(F);
        Weight& nodeWeight = Fnode->getWeight();
        nodeWeight.addFactor(factor);
    }
}

void WeightAssigningHelper::assignNodeSizeWeights()
{
    WeightFactor sizeFactor(WeightFactor::SIZE);
    for (auto it = m_callGraph.begin();
            it != m_callGraph.end();
            ++it) {
        int Fsize = Utils::getFunctionSize(it->first);
        sizeFactor.setValue(Fsize);
        Weight& nodeWeight = it->second->getWeight();
        nodeWeight.addFactor(sizeFactor);
    }
}

void WeightAssigningHelper::assignEdgeWeights()
{
    assignCallNumWeights();
    assignArgWeights();
    assignRetValueWeights();
}

void WeightAssigningHelper::assignCallNumWeights()
{
    const auto& callSiteData = collectFunctionCallSiteData();
    WeightFactor callNumFactor(WeightFactor::CALL_NUM);
    for (auto it = m_callGraph.begin(); it != m_callGraph.end(); ++it) {
        llvm::Function* F = it->first;
        auto functionCallDataPos = callSiteData.find(F);
        if (functionCallDataPos != callSiteData.end()) {
            continue;
        }
        for (auto edge_it = it->second->inEdgesBegin();
             edge_it != it->second->inEdgesEnd();
             ++edge_it) {
             llvm::Function* caller = edge_it->getSink()->getFunction();
             int calls = functionCallDataPos->second.find(caller)->second;
             Weight& edgeWeight = edge_it->getWeight();
             callNumFactor.setValue(calls);
             edgeWeight.addFactor(callNumFactor);
        }
    }
}

void WeightAssigningHelper::assignArgWeights()
{
    WeightFactor argNumFactor(WeightFactor::ARG_NUM);
    WeightFactor argComplexityFactor(WeightFactor::ARG_COMPLEXITY);
    for (auto it = m_callGraph.begin(); it != m_callGraph.end(); ++it) {
        argNumFactor.setValue(it->first->arg_size());
        int argsComplexity = getArgComplexity(it->first);
        argComplexityFactor.setValue(argsComplexity);
        for (auto edge_it = it->second->inEdgesBegin();
             edge_it != it->second->inEdgesEnd();
             ++edge_it) {
             Weight& edgeWeight = edge_it->getWeight();
             edgeWeight.addFactor(argNumFactor);
             edgeWeight.addFactor(argComplexityFactor);
        }
    }
}

void WeightAssigningHelper::assignRetValueWeights()
{
    WeightFactor factor(WeightFactor::RET_COMPLEXITY);
    for (auto it = m_callGraph.begin(); it != m_callGraph.end(); ++it) {
        factor.setValue(getTypeComplexity(it->first->getReturnType()));
        for (auto edge_it = it->second->inEdgesBegin();
                edge_it != it->second->inEdgesEnd();
                ++edge_it) {
            Weight& edgeWeight = edge_it->getWeight();
            edgeWeight.addFactor(factor);
        }
    }
}

WeightAssigningHelper::CallSiteData
WeightAssigningHelper::collectFunctionCallSiteData()
{
    CallSiteData callSiteData;
    for (auto& Fpdg : m_pdg->getFunctionPDGs()) {
        auto& fCallSiteData = callSiteData[Fpdg.first];
        for (const auto& callSite : Fpdg.second->getCallSites()) {
            llvm::Function* caller = callSite.getCaller();
            llvm::LoopInfo* loop = m_loopInfoGetter(caller);
            if (loop && loop->getLoopFor(callSite.getParent())) {
                fCallSiteData[caller] = std::numeric_limits<int>::max();
            } else if (fCallSiteData[caller] != std::numeric_limits<int>::max()) {
                ++fCallSiteData[caller];
            }
        }
    }
    return callSiteData;
}

/**********************************************************/
CallGraph::CallGraph(const llvm::CallGraph& graph)
{
    create(graph);
}

bool CallGraph::hasFunctionNode(llvm::Function* F) const
{
    return m_functionNodes.find(F) != m_functionNodes.end();
}

Node* CallGraph::getFunctionNode(llvm::Function* F) const
{
    assert(hasFunctionNode(F));
    return m_functionNodes.find(F)->second.get();
}

void CallGraph::assignWeights(const Partition& securePartition,
                              const Partition& insecurePartition,
                              const pdg::PDG* pdg,
                              const LoopInfoGetter& loopInfoGetter)
{
    WeightAssigningHelper helper(*this, securePartition, insecurePartition, pdg, loopInfoGetter);
    helper.assignWeights();
}

void CallGraph::create(const llvm::CallGraph& graph)
{
    for (auto it = graph.begin(); it != graph.end(); ++it) {
        if (!it->first) {
            continue;
        }
        Node* node = getOrAddNode(const_cast<llvm::Function*>(it->first));
        addNodeConnections(it->second.get(), node);
    }
}

Node* CallGraph::getOrAddNode(llvm::Function* F)
{
    if (!hasFunctionNode(F)) {
        m_functionNodes.insert(std::make_pair(F, std::make_unique<Node>(F)));
    }
    return getFunctionNode(F);
}

void CallGraph::addNodeConnections(llvm::CallGraphNode* llvmNode, Node* sourceNode)
{
    if (!llvmNode->getFunction()) {
        return;
    }
    Node* sinkNode = getOrAddNode(llvmNode->getFunction());
    Edge edge(sourceNode, sinkNode);
    sourceNode->addOutEdge(edge);
    sinkNode->addInEdge(edge);
}

} //namespace vazgen

