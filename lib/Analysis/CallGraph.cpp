#include "Analysis/CallGraph.h"

#include "Analysis/ProgramPartitionAnalysis.h"
#include "Utils/Utils.h"
#include "PDG/PDG/PDG.h"
#include "PDG/PDG/FunctionPDG.h"
#include "PDG/Passes/PDGBuildPasses.h"

#include "llvm/Analysis/CallGraph.h"
#include "llvm/Analysis/LoopInfo.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/IR/Type.h"
#include "llvm/PassRegistry.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/Transforms/IPO/PassManagerBuilder.h"
#include "llvm/Analysis/DOTGraphTraitsPass.h"

#include <sstream>

namespace llvm {

template <> struct GraphTraits<vazgen::Node*>
{
  using NodeRef = vazgen::Node*;
  using DerefEdge = std::pointer_to_unary_function<vazgen::Edge, NodeRef>;
  using ChildIteratorType = mapped_iterator<vazgen::Node::iterator, DerefEdge>;

  static NodeRef getEntryNode(vazgen::Node *CGN) { return CGN; }

  static ChildIteratorType child_begin(NodeRef N) {
    return map_iterator(N->outEdgesBegin(), DerefEdge(edgeDereference));
  }

  static ChildIteratorType child_end(NodeRef N) {
    return map_iterator(N->outEdgesEnd(), DerefEdge(edgeDereference));
  }

  static NodeRef edgeDereference(vazgen::Edge edge) {
      return edge.getSink();
  }

};

template <> struct GraphTraits<const vazgen::Node*>
{
  using NodeRef = const vazgen::Node*;
  using DerefEdge = std::pointer_to_unary_function<vazgen::Edge, NodeRef>;
  using ChildIteratorType = mapped_iterator<vazgen::Node::const_iterator, DerefEdge>;

  static NodeRef getEntryNode(const vazgen::Node *CGN) { return CGN; }

  static ChildIteratorType child_begin(NodeRef N) {
    return map_iterator(N->outEdgesBegin(), DerefEdge(edgeDereference));
  }

  static ChildIteratorType child_end(NodeRef N) {
    return map_iterator(N->outEdgesEnd(), DerefEdge(edgeDereference));
  }
  static NodeRef edgeDereference(const vazgen::Edge edge) {
      return edge.getSink();
  }

};

template <>
struct GraphTraits<vazgen::CallGraph *> : public GraphTraits<vazgen::Node *> {
  using PairTy = std::pair<Function *const, vazgen::CallGraph::NodeType>;

  static NodeRef getEntryNode(vazgen::CallGraph *CGN) {
      return CGN->begin()->second.get();
  }

  static vazgen::Node *CGGetValuePtr(const PairTy &P) {
      return P.second.get();
  }

  // nodes_iterator/begin/end - Allow iteration over all nodes in the graph
  using nodes_iterator =
      mapped_iterator<vazgen::CallGraph::iterator, decltype(&CGGetValuePtr)>;

  static nodes_iterator nodes_begin(vazgen::CallGraph *CG) {
    return nodes_iterator(CG->begin(), &CGGetValuePtr);
  }

  static nodes_iterator nodes_end(vazgen::CallGraph *CG) {
    return nodes_iterator(CG->end(), &CGGetValuePtr);
  }
};

template <>
struct GraphTraits<const vazgen::CallGraph *> : public GraphTraits<const vazgen::Node*> {
  using PairTy = std::pair<Function *const, vazgen::CallGraph::NodeType>;

  static NodeRef getEntryNode(const vazgen::CallGraph *CGN) {
      return CGN->begin()->second.get();
  }

  static const vazgen::Node *CGGetValuePtr(const PairTy &P) {
    return P.second.get();
  }

  // nodes_iterator/begin/end - Allow iteration over all nodes in the graph
  using nodes_iterator =
      mapped_iterator<vazgen::CallGraph::const_iterator, decltype(&CGGetValuePtr)>;

  static nodes_iterator nodes_begin(const vazgen::CallGraph *CG) {
    return nodes_iterator(CG->begin(), &CGGetValuePtr);
  }

  static nodes_iterator nodes_end(const vazgen::CallGraph *CG) {
    return nodes_iterator(CG->end(), &CGGetValuePtr);
  }
};

template <> struct DOTGraphTraits<vazgen::CallGraph *> : public DefaultDOTGraphTraits {
    typedef GraphTraits<vazgen::CallGraph*>::NodeRef NodeRef;
    typedef GraphTraits<vazgen::CallGraph*>::ChildIteratorType ChildIteratorType;

  DOTGraphTraits(bool isSimple = false) : DefaultDOTGraphTraits(isSimple) {}

  static std::string getGraphName(vazgen::CallGraph *Graph) { return "Call graph"; }

  std::string getNodeLabel(NodeRef node, vazgen::CallGraph *Graph) {
      std::stringstream label;
      label << node->getFunction()->getName().str();
      if (node->getWeight().hasFactor(vazgen::WeightFactor::SIZE)) {
        label << " size " << std::to_string((int)node->getWeight().getFactor(vazgen::WeightFactor::SIZE).getValue());
      }
      if (node->getWeight().hasFactor(vazgen::WeightFactor::SENSITIVE_RELATED)) {
        auto value = node->getWeight().getFactor(vazgen::WeightFactor::SENSITIVE_RELATED).getValue();
        if (value.isPosInfinity()) {
            label << " sensitive related level infinity";
        } else {
            label << " sensitive related level " << std::to_string(value);
        }
      }
      return label.str();
  }

  static std::string getNodeAttributes(NodeRef node, vazgen::CallGraph* graph)
  {
    if (node->getWeight().hasFactor(vazgen::WeightFactor::SENSITIVE)) {
        return "color=green";
    } else if (node->getFunction()->isDeclaration()) {
        return "color=red";
    }
    return "color=black";
  }

  static std::string getEdgeAttributes(NodeRef node, ChildIteratorType edge_iter, vazgen::CallGraph* graph)
  {
      return "";
  }

  static std::string getEdgeSourceLabel(NodeRef node, ChildIteratorType edge_iter)
  {
    std::stringstream label;
    auto edge = edge_iter.getCurrent();
    const auto& edgeWeight = edge->getWeight();
    if (edgeWeight.hasFactor(vazgen::WeightFactor::CALL_NUM)) {
        auto value = edgeWeight.getFactor(vazgen::WeightFactor::CALL_NUM).getValue();
        if (value.isPosInfinity()) {
            label << "loop";
        } else {
            label << std::to_string((int)value);
        }
    }
    return label.str();
  }

};

struct AnalysisCallGraphPassTraits {
  static vazgen::CallGraph *getGraph(vazgen::CallGraphPass *P) {
    return &P->getCallGraph();
  }
};

} // end llvm namespace

namespace vazgen {

static const int LOOP_COST = 100000;

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
    case WeightFactor::SENSITIVE_RELATED:
        return "sensitive_related";
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
    using CallSiteData = std::unordered_map<llvm::Function*, std::unordered_map<llvm::Function*, Double>>;

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
    void assignSensitiveRelatedNodeWeights();
    void assignNodeSizeWeights();
    void assignEdgeWeights();
    void assignCallNumWeights();
    void assignArgWeights();
    void assignRetValueWeights();
    CallSiteData collectFunctionCallSiteData();
    void normalizeWeight();

private:
    CallGraph& m_callGraph;
    const Partition& m_securePartition;
    const Partition& m_insecurePartition;
    const pdg::PDG* m_pdg;
    const LoopInfoGetter& m_loopInfoGetter;
    std::vector<Double*> m_allWeights;
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
    normalizeWeight();
}

void WeightAssigningHelper::assignNodeWeights()
{
    assignSensitiveNodeWeights();
    assignSensitiveRelatedNodeWeights();
    assignNodeSizeWeights();
}

void WeightAssigningHelper::assignSensitiveNodeWeights()
{
    WeightFactor factor(WeightFactor::SENSITIVE);
    factor.setValue(Double::POS_INFINITY);
    for (llvm::Function* F : m_securePartition.getPartition()) {
        auto* Fnode = m_callGraph.getFunctionNode(F);
        Weight& nodeWeight = Fnode->getWeight();
        nodeWeight.addFactor(factor);
        m_allWeights.push_back(&nodeWeight.getFactor(WeightFactor::SENSITIVE).getValue());
    }
}

void WeightAssigningHelper::assignSensitiveRelatedNodeWeights()
{
    WeightFactor factor(WeightFactor::SENSITIVE_RELATED);
    factor.setValue(Double::POS_INFINITY);
    for (const auto& [function, level] : m_securePartition.getRelatedFunctions()) {
        factor.setCoef(1/level);
        auto* Fnode = m_callGraph.getFunctionNode(function);
        Weight& nodeWeight = Fnode->getWeight();
        nodeWeight.addFactor(factor);
        m_allWeights.push_back(&nodeWeight.getFactor(WeightFactor::SENSITIVE_RELATED).getValue());
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
        m_allWeights.push_back(&nodeWeight.getFactor(WeightFactor::SIZE).getValue());
    }
}

void WeightAssigningHelper::assignEdgeWeights()
{
    // TODO: for each make sure to assign both to in edges and out edges
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
        if (functionCallDataPos == callSiteData.end()) {
            continue;
        }
        for (auto edge_it = it->second->inEdgesBegin();
             edge_it != it->second->inEdgesEnd();
             ++edge_it) {
             llvm::Function* caller = edge_it->getSource()->getFunction();
             int calls = functionCallDataPos->second.find(caller)->second;
             Weight& edgeWeight = edge_it->getWeight();
             callNumFactor.setValue(calls);
             edgeWeight.addFactor(callNumFactor);
             m_allWeights.push_back(&edgeWeight.getFactor(WeightFactor::CALL_NUM).getValue());
        }
    }

    // This makes edge updating more complicated, plus edges are duplicated.
    // See if can have one direction of connection, i.e. only outEdges
    for (auto it = m_callGraph.begin(); it != m_callGraph.end(); ++it) {
        llvm::Function* caller = it->first;
        for (auto edge_it = it->second->outEdgesBegin();
                edge_it != it->second->outEdgesEnd();
                ++edge_it) {
            llvm::Function* F = edge_it->getSink()->getFunction();
            auto functionCallDataPos = callSiteData.find(F);
            if (functionCallDataPos == callSiteData.end()) {
                continue;
            }
            auto callerPos = functionCallDataPos->second.find(caller);
            if (callerPos == functionCallDataPos->second.end()) {
                continue;
            }
            int calls = callerPos->second;
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
             m_allWeights.push_back(&edgeWeight.getFactor(WeightFactor::ARG_NUM).getValue());
             m_allWeights.push_back(&edgeWeight.getFactor(WeightFactor::ARG_COMPLEXITY).getValue());
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
            m_allWeights.push_back(&edgeWeight.getFactor(WeightFactor::RET_COMPLEXITY).getValue());
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
                //fCallSiteData[caller] = Double::POS_INFINITY;
                fCallSiteData[caller] = LOOP_COST;
            } else if (!fCallSiteData[caller].isPosInfinity()) {
                ++fCallSiteData[caller];
            }
        }
    }
    return callSiteData;
}

void WeightAssigningHelper::normalizeWeight()
{
    // TODO:
//    Double minNonInfinityWeigth;
//    Double maxNonInfinityWeight;
//    for (const auto& weight : m_allWeights) {
//        if (minNonInfinityWeigth > *weight && !weight->isNegInfinity()) {
//            minNonInfinityWeigth = *weight;
//        }
//        if (maxNonInfinityWeight < *weight && !weight->isPosInfinity()) {
//            maxNonInfinityWeight = *weight;
//        }
//    }
//    for (const auto& weight : m_allWeights) {
//        if (*weight > )
//    }
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
        for (auto conn_it = it->second->begin(); conn_it != it->second->end(); ++conn_it) {
            addNodeConnections(conn_it->second, node);
        }
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
    if (sourceNode->addOutEdge(edge)) {
        sinkNode->addInEdge(edge);
    }
}

char CallGraphPass::ID = 0;

void CallGraphPass::getAnalysisUsage(llvm::AnalysisUsage& AU) const
{
    AU.addRequired<pdg::SVFGPDGBuilder>();
    AU.addRequired<llvm::CallGraphWrapperPass>();
    AU.addPreserved<llvm::CallGraphWrapperPass>();
    AU.addRequired<llvm::LoopInfoWrapperPass>();
    AU.addRequired<ProgramPartitionAnalysis>();
}

bool CallGraphPass::runOnModule(llvm::Module& M)
{
    llvm::CallGraph& CG = getAnalysis<llvm::CallGraphWrapperPass>().getCallGraph();
    m_callgraph.reset(new CallGraph(CG));
    auto* partition = &getAnalysis<vazgen::ProgramPartitionAnalysis>().getProgramPartition();
    auto pdg = getAnalysis<pdg::SVFGPDGBuilder>().getPDG();
    const auto& loopGetter = [this] (llvm::Function* F)
        {   return &this->getAnalysis<llvm::LoopInfoWrapperPass>(*F).getLoopInfo(); };

    m_callgraph->assignWeights(partition->getSecurePartition(), partition->getInsecurePartition(),
                               pdg.get(), loopGetter);
    return false;
}

class CallGraphDotPrinter : public llvm::DOTGraphTraitsModulePrinter<CallGraphPass,
                                                                     true,
                                                                     CallGraph*,
                                                                     llvm::AnalysisCallGraphPassTraits>
{
public:
    static char ID;

    CallGraphDotPrinter()
        : DOTGraphTraitsModulePrinter<CallGraphPass, true, CallGraph *,
                                      llvm::AnalysisCallGraphPassTraits>("callgraph", ID)
    {
        llvm::initializeCallGraphDOTPrinterPass(*llvm::PassRegistry::getPassRegistry());
    }
}; // class CallGraphDotPrinter


char CallGraphDotPrinter::ID = 0;
static llvm::RegisterPass<CallGraphPass> X("callgraph","Creates Call graph");
static llvm::RegisterPass<CallGraphDotPrinter> Y("callgraph-dot","Prints Call graph");

} //namespace vazgen

