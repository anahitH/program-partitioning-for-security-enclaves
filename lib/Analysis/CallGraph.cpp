#include "Analysis/CallGraph.h"

#include "llvm/IR/Function.h"
#include "llvm/Analysis/CallGraph.h"

namespace vazgen {

// TODO: implement
class CallGraph::NodeWeight
{
public:

private:

}; // class NodeWeight

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

private:
    Node* m_source;
    Node* m_sink;
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

} //namespace vazgen

