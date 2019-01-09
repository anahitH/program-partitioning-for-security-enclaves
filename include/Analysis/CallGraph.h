#pragma once

#include <memory>
#include <unordered_map>

namespace llvm {
class Function;
class CallGraph;
class CallGraphNode;
}

namespace vazgen {

class CallGraph
{
public:
    class Node;
    class Edge;
    class NodeWeight;

    using NodeType = std::unique_ptr<Node>;
    using FunctionNodes = std::unordered_map<llvm::Function*, NodeType>;

public:
    explicit CallGraph(const llvm::CallGraph& graph);

    CallGraph(const CallGraph&) = delete;
    CallGraph(CallGraph&&) = delete;
    CallGraph& operator =(const CallGraph&) = delete;
    CallGraph& operator =(CallGraph&&) = delete;

public:
    bool hasFunctionNode(llvm::Function* F) const;
    Node* getFunctionNode(llvm::Function* F) const;

private:    
    void create(const llvm::CallGraph& graph);
    Node* getOrAddNode(llvm::Function* F);
    void addNodeConnections(llvm::CallGraphNode* llvmNode,
                            Node* sourceNode);

private:
    FunctionNodes m_functionNodes;
}; // class CallGraph

} // namespace vazgen

