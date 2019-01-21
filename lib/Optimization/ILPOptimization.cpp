#include "Optimization/ILPOptimization.h"

#include "Analysis/CallGraph.h"
#include "Analysis/Partition.h"
#include "Utils/Logger.h"

#include "llvm/IR/Function.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/raw_ostream.h"

#include "ilcplex/ilocplex.h"

using namespace std;

namespace vazgen {

class ILPOptimization::Impl
{
public:
    Impl(const CallGraph& callgraph,
         Partition& partition,
         Logger& logger);

public:
    void run();

private:
    void createNodeVariables();
    void createEdgeVariables();
    void createEdgeVariables(Node* node);
    void createConstraints();
    void createObjective();

private:
    const CallGraph& m_callgraph;
    Partition& m_partition;
    Logger& m_logger;

    IloEnv m_ilpEnv;
    IloModel m_ilpModel;
    std::unordered_map<Node*, IloNumVar> m_nodeVariables;
    std::unordered_map<Edge*, IloNumVar> m_edgeVariables;
    std::unordered_map<llvm::Function*, int> m_functionIdx;
}; // class Impl

ILPOptimization::Impl::Impl(const CallGraph& callgraph,
                            Partition& partition,
                            Logger& logger)
    : m_callgraph(callgraph)
    , m_partition(partition)
    , m_logger(logger)
    , m_ilpModel(m_ilpEnv)
{
}

void ILPOptimization::Impl::run()
{
    createNodeVariables();
    createEdgeVariables();
    createConstraints();
    createObjective();
    IloCplex cplex(m_ilpModel);
    cplex.exportModel("partition_model.lp");

    if ( !cplex.solve() ) {
        m_ilpEnv.error() << "Failed to optimize LP" << endl;
        return;
    }
    m_logger.info("Yuhuuu");
}

void ILPOptimization::Impl::createNodeVariables()
{
    int i = 0;
    for (auto it = m_callgraph.begin(); it != m_callgraph.end(); ++it) {
        m_nodeVariables.insert(std::make_pair(it->second.get(),
                                              IloNumVar(m_ilpEnv, 0.0, 1.0)));
        m_functionIdx.insert(std::make_pair(it->second->getFunction(), i));
        const std::string name = "v" + std::to_string(i++);
        m_nodeVariables[it->second.get()].setName(name.c_str());
    }
}

// For out edges only
void ILPOptimization::Impl::createEdgeVariables()
{
    for (auto it = m_callgraph.begin(); it != m_callgraph.end(); ++it) {
        createEdgeVariables(it->second.get());
    }
}

void ILPOptimization::Impl::createEdgeVariables(Node* node)
{
    for (auto it = node->outEdgesBegin(); it != node->outEdgesEnd(); ++it) {
        Edge* edge = &*it;
        auto [itr, inserted] = m_edgeVariables.insert(
                std::make_pair(edge, IloNumVar(m_ilpEnv, 0.0, 1.0)));
        assert(inserted);
        const std::string name = "f"
                + std::to_string(m_functionIdx[edge->getSource()->getFunction()])
                + std::to_string(m_functionIdx[edge->getSink()->getFunction()]);
        itr->second.setName(name.c_str());
    }
}

void ILPOptimization::Impl::createConstraints()
{
    for (const auto& [node, var] : m_nodeVariables) {
        auto* F = node->getFunction();
        if (m_partition.contains(F)) {
            m_ilpModel.add(var <= 1);
            m_ilpModel.add(var >= 1);
        } else if (F->isDeclaration() || F->getName() == "main") {
            m_ilpModel.add(var <= 0);
        }
    }
    for (const auto& [edge, var] : m_edgeVariables) {
        const auto& source_var = m_nodeVariables.find(edge->getSource())->second;
        const auto& sink_var = m_nodeVariables.find(edge->getSink())->second;
        m_ilpModel.add(var - source_var + sink_var <= 1);
        m_ilpModel.add(var + source_var - sink_var <= 1);
    }
}

void ILPOptimization::Impl::createObjective()
{
    IloObjective obj = IloMaximize(m_ilpEnv);
    for (const auto& [edge, val] : m_edgeVariables) {
        const auto& edgeCost = edge->getWeight().getValue();
        obj.setLinearCoef(val, edgeCost);
    }
    m_ilpModel.add(obj);
}

ILPOptimization::ILPOptimization(const CallGraph& callgraph,
                                 Partition& partition,
                                 Logger& logger)
    : PartitionOptimization(partition, nullptr, logger, PartitionOptimizer::ILP)
    , m_impl(new Impl(callgraph, partition, logger))
{
}

void ILPOptimization::run()
{
    m_impl->run();
}

} // namesapce vazgen

