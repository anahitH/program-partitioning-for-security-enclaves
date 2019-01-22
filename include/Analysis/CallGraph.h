#pragma once

#include "Analysis/Partition.h"
#include "Analysis/Numbers.h"

#include "llvm/Pass.h"

#include <functional>
#include <memory>
#include <unordered_map>
#include <vector>

namespace llvm {
class Function;
class CallGraph;
class CallGraphNode;
class LoopInfo;
}

namespace pdg {
class PDG;
}

namespace vazgen {

class WeightFactor
{
public:
    enum Factor {
        SENSITIVE = 0,
        SENSITIVE_RELATED,
        SIZE,
        CALL_NUM,
        ARG_NUM,
        ARG_COMPLEXITY,
        RET_COMPLEXITY,
        UNKNOWN
    };
public:
    explicit WeightFactor(Factor factor = UNKNOWN)
        : m_factor(factor)
        , m_coeff(1.0)
        , m_value(0)
    {
    }

public:
    Factor getFactor() const
    {
        return m_factor;
    }

    bool isDefined() const
    {
        return m_factor < UNKNOWN;
    }

    void setValue(double value)
    {
        m_value = value;
    }

    void setCoef(Double coef)
    {
        m_coeff = coef;
    }

    Double getValue() const
    {
        return m_value;
    }

    Double getCoef() const
    {
        return m_coeff;
    }

    Double getWeight() const
    {
        return m_value * m_coeff;
    }

private:
    Factor m_factor;
    std::string m_name;
    Double m_value;
    Double m_coeff;
}; // class WeightFactor

class Weight
{
public:
    using Factors = std::vector<WeightFactor>;

public:
    Weight()
        : m_factors(WeightFactor::UNKNOWN)
    {
    }

    void addFactor(const WeightFactor& factor)
    {
        m_factors[factor.getFactor()] = factor;
    }

    bool hasFactor(WeightFactor::Factor factor) const
    {
        return m_factors[factor].isDefined();
    }

    WeightFactor& getFactor(WeightFactor::Factor factor)
    {
        return m_factors[factor];
    }

    const WeightFactor& getFactor(WeightFactor::Factor factor) const
    {
        return const_cast<Weight*>(this)->getFactor(factor);
    }

    Double getValue() const
    {
        Double weight = 0.0;
        for (const auto& factor : m_factors) {
            if (factor.isDefined()) {
                weight += factor.getWeight();
            }
        }
        return weight;
    }

private:
    Factors m_factors;
}; // class Weight

class Node;

class Edge
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
}; //class Node

class Node
{
public:
    using Edges = std::vector<Edge>;
    using iterator = Edges::iterator;
    using const_iterator = Edges::const_iterator;
    using children_iterator = std::unordered_set<Node*>::iterator;
    using const_children_iterator = std::unordered_set<Node*>::const_iterator;

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

    bool addOutEdge(Edge edge)
    {
        auto [iter, inserted] = m_children.insert(edge.getSink());
        if (inserted) {
            m_outEdges.push_back(edge);
        }
        return inserted;
    }

    void connectTo(Node* node)
    {
        Edge edge(this, node);
        auto [iter, inserted] = m_children.insert(node);
        if (inserted) {
            m_outEdges.push_back(edge);
        }
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

    children_iterator children_begin()
    {
        return m_children.begin();
    }

    children_iterator children_end()
    {
        return m_children.end();
    }

    const_children_iterator children_begin() const
    {
        return m_children.begin();
    }

    const_children_iterator children_end() const
    {
        return m_children.end();
    }

private:
    llvm::Function* m_F;
    Edges m_inEdges;
    Edges m_outEdges;
    std::unordered_set<Node*> m_children;
    Weight m_weight;
}; //class Node

class CallGraph
{
public:
    using NodeType = std::unique_ptr<Node>;
    using FunctionNodes = std::unordered_map<llvm::Function*, NodeType>;
    using LoopInfoGetter = std::function<llvm::LoopInfo* (llvm::Function*)>;
    using iterator = FunctionNodes::iterator;
    using const_iterator = FunctionNodes::const_iterator;

public:
    explicit CallGraph(const llvm::CallGraph& graph);

    CallGraph(const CallGraph&) = delete;
    CallGraph(CallGraph&&) = delete;
    CallGraph& operator =(const CallGraph&) = delete;
    CallGraph& operator =(CallGraph&&) = delete;

public:
    bool hasFunctionNode(llvm::Function* F) const;
    Node* getFunctionNode(llvm::Function* F) const;

    void assignWeights(const Partition& securePartition,
                       const Partition& insecurePartition,
                       const pdg::PDG* pdg,
                       const LoopInfoGetter& loopInfoGetter);

public:
    iterator begin()
    {
        return m_functionNodes.begin();
    }

    iterator end()
    {
        return m_functionNodes.end();
    }

    const_iterator begin() const
    {
        return m_functionNodes.begin();
    }

    const_iterator end() const
    {
        return m_functionNodes.end();
    }

private:    
    void create(const llvm::CallGraph& graph);
    Node* getOrAddNode(llvm::Function* F);
    void addNodeConnections(llvm::CallGraphNode* llvmNode,
                            Node* sourceNode);

private:
    FunctionNodes m_functionNodes;
}; // class CallGraph

class CallGraphPass : public llvm::ModulePass
{
public:
    static char ID;

    CallGraphPass()
        : llvm::ModulePass(ID)
    {
    }

    void getAnalysisUsage(llvm::AnalysisUsage& AU) const override;
    bool runOnModule(llvm::Module& M) override;

    const CallGraph& getCallGraph() const
    {
        return *m_callgraph;
    }

    CallGraph& getCallGraph()
    {
        return *m_callgraph;
    }

private:
    std::unique_ptr<CallGraph> m_callgraph;
}; // class CallGraphPass

} // namespace vazgen

