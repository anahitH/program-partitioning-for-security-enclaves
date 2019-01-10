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

std::string getNodeFactorName(CallGraph::NodeFactor fact)
{
    switch (fact) {
    case CallGraph::ANNOTATED_SENSITIVE:
        return "annotated_sensitive";
    case CallGraph::ANALYSIS_SENSITIVE:
        return "analysis_sensitive";
    case CallGraph::SIZE:
        return "size";
    default:
        assert(false);
    };
    return "";
}

std::string getEdgeFactorName(CallGraph::EdgeFactor fact)
{
    switch (fact) {
    case CallGraph::IN_LOOP:
        return "in_loop";
    case CallGraph::ARG_NUM:
        return "arg_num";
    case CallGraph::ARG_COMPLEXITY:
        return "arg_complexity";
    case CallGraph::RET_COMPLEXITY:
        return "ret_complexity";
    default:
        assert(false);
    };
    return "";
}

} // unnamed namespace

class CallGraph::WeightFactor
{
public:
    WeightFactor(const std::string name)
        : m_name(name)
    {
    }

public:
    void setValue(int value)
    {
        m_value = value;
    }

    void setCoef(double coef)
    {
        m_coeff = coef;
    }

    int getValue() const
    {
        return m_value;
    }

    double getCoef() const
    {
        return m_coeff;
    }

    double getWeight() const
    {
        //TODO: consider having a member for this
        return m_value * m_coeff;
    }

private:
    std::string m_name;
    int m_value;
    double m_coeff;
};

class CallGraph::Weight
{
public:
    using Factors = std::unordered_map<std::string, WeightFactor>;

public:
    Weight() = default;


    void addFactor(const std::string& name, const WeightFactor& factor)
    {
        m_factors.insert(std::make_pair(name, factor));
    }

    bool hasFactor(const std::string& name) const
    {
        return m_factors.find(name) != m_factors.end();
    }

    WeightFactor& getFactor(const std::string& name)
    {
        return m_factors.find(name)->second;
    }

    const WeightFactor& getFactor(const std::string& name) const
    {
        return const_cast<Weight*>(this)->getFactor(name);
    }

    double getWeight() const
    {
        double weight = 0.0;
        for (const auto& factor : m_factors) {
            weight += factor.second.getWeight();
        }
        return weight;
    }

private:
    Factors m_factors;
};

class CallGraph::Edge
{
public:
    Edge(Node* source, Node* sink)
        : m_source(source)
        , m_sink(sink)
    {
    }

public:
    Node* getSource() const
    {
        return m_source;
    }

    Node* getSink() const
    {
        return m_sink;
    }

    Weight& getWeight()
    {
        return m_weight;
    }

    const Weight& getWeight() const
    {
        return const_cast<Edge*>(this)->getWeight();
    }

private:
    Node* m_source;
    Node* m_sink;
    Weight m_weight;
}; //class CallGraph::Node

class CallGraph::Node
{
public:
    using Edges = std::vector<CallGraph::Edge>;
    using iterator = Edges::iterator;
    using const_iterator = Edges::const_iterator;

public:
    Node(llvm::Function* F)
        : m_F(F)
    {
    }

    Node(const Node&) = delete;
    Node(Node&&) = delete;
    Node& operator =(const Node&) = delete;
    Node& operator =(Node&&) = delete;

public:
    llvm::Function* getFunction() const
    {
        return m_F;
    }

    const Edges& getInEdges() const
    {
        return m_inEdges;
    }

    const Edges& getOutEdges() const
    {
        return m_outEdges;
    }

    void addInEdge(Edge edge)
    {
        m_inEdges.push_back(edge);
    }

    void addOutEdge(Edge edge)
    {
        m_outEdges.push_back(edge);
    }

    void connectTo(CallGraph::Node* node)
    {
        Edge edge(this, node);
        addOutEdge(edge);
    }

    Weight& getWeight()
    {
        return m_weight;
    }

    const Weight& getWeight() const
    {
        return const_cast<Node*>(this)->getWeight();
    }

public:
    iterator inEdgesBegin()
    {
        return m_inEdges.begin();
    }

    iterator inEdgesEnd()
    {
        return m_inEdges.end();
    }

    const_iterator inEdgesBegin()const
    {
        return m_inEdges.begin();
    }

    const_iterator inEdgesEnd() const
    {
        return m_inEdges.end();
    }

    iterator outEdgesBegin()
    {
        return m_outEdges.begin();
    }

    iterator outEdgesEnd()
    {
        return m_outEdges.end();
    }

    const_iterator outEdgesBegin()const
    {
        return m_outEdges.begin();
    }

    const_iterator outEdgesEnd() const
    {
        return m_outEdges.end();
    }

private:
    llvm::Function* m_F;
    Edges m_inEdges;
    Edges m_outEdges;
    Weight m_weight;
}; //class CallGraph::Node

CallGraph::CallGraph(const llvm::CallGraph& graph)
{
    create(graph);
}

bool CallGraph::hasFunctionNode(llvm::Function* F) const
{
    return m_functionNodes.find(F) != m_functionNodes.end();
}

CallGraph::Node* CallGraph::getFunctionNode(llvm::Function* F) const
{
    assert(hasFunctionNode(F));
    return m_functionNodes.find(F)->second.get();
}

void CallGraph::assignWeights(const Partition::FunctionSet& annotatedFs,
                              const Partition& securePartition,
                              const Partition& insecurePartition,
                              const pdg::PDG* pdg,
                              const LoopInfoGetter& loopInfoGetter)
{
    assignNodeWeights(annotatedFs, securePartition, insecurePartition);
    assignEdgeWeights(pdg, loopInfoGetter);
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

CallGraph::Node* CallGraph::getOrAddNode(llvm::Function* F)
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

void CallGraph::assignNodeWeights(const Partition::FunctionSet& annotatedFs,
                                  const Partition& securePartition,
                                  const Partition& insecurePartition)
{
    assignAnnotatedFunctionsWeights(annotatedFs);
    assignAnalysisFunctionsWeights(annotatedFs, securePartition);
    assignSizeWeights();
}

void CallGraph::assignAnnotatedFunctionsWeights(const Partition::FunctionSet& annotatedFs)
{
    const std::string factorName = getNodeFactorName(ANNOTATED_SENSITIVE);
    WeightFactor factor(factorName);
    // TODO, max may be changed with min, depending on how eventually we'll encode the optimization problem
    factor.setValue(std::numeric_limits<int>::max());
    factor.setCoef(1.0);
    for (llvm::Function* annotatedF : annotatedFs) {
        Node* Fnode = getFunctionNode(annotatedF);
        Weight& nodeWeight = Fnode->getWeight();
        nodeWeight.addFactor(factorName, factor);
    }
}

void CallGraph::assignAnalysisFunctionsWeights(const Partition::FunctionSet& annotatedFs,
                                               const Partition& securePartition)
{
    const std::string factorName = getNodeFactorName(ANALYSIS_SENSITIVE);
    WeightFactor factor(factorName);
    // TODO, max may be changed with min, depending on how eventually we'll encode the optimization problem
    // TODO: think about value
    factor.setValue(std::numeric_limits<int>::max() - 1);
    // TODO: think about coefficient
    factor.setCoef(1.0);
    for (llvm::Function* F : securePartition.getPartition()) {
        if (annotatedFs.find(F) != annotatedFs.end()) {
            continue;
        }
        Node* Fnode = getFunctionNode(F);
        Weight& nodeWeight = Fnode->getWeight();
        nodeWeight.addFactor(factorName, factor);
    }
}

void CallGraph::assignSizeWeights()
{
    const std::string factorName = getNodeFactorName(SIZE);
    WeightFactor factor(factorName);
    // TODO: think about coefficient
    factor.setCoef(0.001);
    for (auto& node : m_functionNodes) {
        // TODO: this is size of blocks, consider having size of instructions
        llvm::Function* F = node.first;
        factor.setValue(Utils::getFunctionSize(F));
        Node* Fnode = getFunctionNode(F);
        Weight& nodeWeight = Fnode->getWeight();
        nodeWeight.addFactor(factorName, factor);
    }
}

void CallGraph::assignEdgeWeights(const pdg::PDG* pdg,
                                  const LoopInfoGetter& loopInfoGetter)
{
    assignArgWeights();
    assignRetValueWeights();
    assignLoopEdgeWeights(pdg, loopInfoGetter);
}

void CallGraph::assignArgWeights()
{
    const std::string argNumFactorName = getEdgeFactorName(ARG_NUM);
    const std::string argComplexityFactorName = getEdgeFactorName(ARG_COMPLEXITY);
    WeightFactor argNumFactor(argNumFactorName);
    WeightFactor argComplexityFactor(argComplexityFactorName);
    // TODO: think about coefficient
    argNumFactor.setCoef(0.01);
    argComplexityFactor.setCoef(0.1);
    for (auto& node : m_functionNodes) {
        argNumFactor.setValue(node.first->arg_size());
        int argsComplexity = getArgComplexity(node.first);
        argComplexityFactor.setValue(argsComplexity);
        for (auto it = node.second->inEdgesBegin();
             it != node.second->inEdgesEnd();
             ++it) {
             Weight& edgeWeight = it->getWeight();
             edgeWeight.addFactor(argNumFactorName, argNumFactor);
             edgeWeight.addFactor(argComplexityFactorName, argComplexityFactor);
        }
    }
}

void CallGraph::assignRetValueWeights()
{
    const std::string factorName = getEdgeFactorName(RET_COMPLEXITY);
    WeightFactor factor(factorName);
    factor.setCoef(0.1);
    for (auto& node : m_functionNodes) {
        factor.setValue(getTypeComplexity(node.first->getReturnType()));
        for (auto it = node.second->inEdgesBegin();
             it != node.second->inEdgesEnd();
             ++it) {
            Weight& edgeWeight = it->getWeight();
            edgeWeight.addFactor(factorName, factor);
        }
    }
}

void CallGraph::assignLoopEdgeWeights(const pdg::PDG* pdg,
                                      const LoopInfoGetter& loopInfoGetter)
{
    //TODO:
//    for (auto& node : m_functionNodes) {
//        llvm::Function* F = node.first;
//        auto Fpdg = pdg->getFunctionPDG(F);
//        const auto& callSites = Fpdg->getCallSites();
//        for (auto& callSite : callSites) {
//            llvm::Function* caller = callSite.getCaller();
//            auto* loopInfo = loopInfoGetter(caller);
//            auto* callerBlock = callSite.getParent();
//            if (loopInfo->getLoopFor(callerBlock)) {
//            }
//        }
//    }
}

} //namespace vazgen

