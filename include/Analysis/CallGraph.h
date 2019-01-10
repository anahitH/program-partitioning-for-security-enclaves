#pragma once

#include "Analysis/Partition.h"

#include <memory>
#include <unordered_map>
#include <functional>

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

class CallGraph
{
public:
    class Node;
    class Edge;
    class WeightFactor;
    class Weight;

    using NodeType = std::unique_ptr<Node>;
    using FunctionNodes = std::unordered_map<llvm::Function*, NodeType>;
    using LoopInfoGetter = std::function<llvm::LoopInfo* (llvm::Function*)>;

    enum NodeFactor {
        ANNOTATED_SENSITIVE,
        ANALYSIS_SENSITIVE,
        SIZE,
        NODE_FACTOR_NUM,
    };

    enum EdgeFactor {
        IN_LOOP,
        ARG_NUM,
        ARG_COMPLEXITY,
        RET_COMPLEXITY,
        EDGE_FACTOR_NUM,
    };

public:
    explicit CallGraph(const llvm::CallGraph& graph);

    CallGraph(const CallGraph&) = delete;
    CallGraph(CallGraph&&) = delete;
    CallGraph& operator =(const CallGraph&) = delete;
    CallGraph& operator =(CallGraph&&) = delete;

public:
    bool hasFunctionNode(llvm::Function* F) const;
    Node* getFunctionNode(llvm::Function* F) const;

    void assignWeights(const Partition::FunctionSet& annotatedFs,
                       const Partition& securePartition,
                       const Partition& insecurePartition,
                       const pdg::PDG* pdg,
                       const LoopInfoGetter& loopInfoGetter);

private:    
    void create(const llvm::CallGraph& graph);
    Node* getOrAddNode(llvm::Function* F);
    void addNodeConnections(llvm::CallGraphNode* llvmNode,
                            Node* sourceNode);
    void assignNodeWeights(const Partition::FunctionSet& annotatedFs,
                           const Partition& securePartition,
                           const Partition& insecurePartition);
    void assignAnnotatedFunctionsWeights(const Partition::FunctionSet& annotatedFs);
    void assignAnalysisFunctionsWeights(const Partition::FunctionSet& annotatedFs,
                                        const Partition& securePartition);
    void assignSizeWeights();
    void assignEdgeWeights(const pdg::PDG* pdg, const LoopInfoGetter& loopInfoGetter);
    void assignArgWeights();
    void assignRetValueWeights();
    void assignLoopEdgeWeights(const pdg::PDG* pdg, const LoopInfoGetter& loopInfoGetter);

private:
    FunctionNodes m_functionNodes;
}; // class CallGraph

} // namespace vazgen

