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
         Partition& securePartition,
         Partition& insecurePartition,
         Logger& logger);

public:
    void run();
    void apply();

private:
    void createNodeVariables();
    void createEdgeVariables();
    void createEdgeVariables(Node* node);
    void createConstraints();
    void createObjective();

private:
    const CallGraph& m_callgraph;
    Partition& m_securePartition;
    Partition& m_insecurePartition;
    Logger& m_logger;

    IloEnv m_ilpEnv;
    IloModel m_ilpModel;
    IloCplex m_cplex;
    std::unordered_map<Node*, IloNumVar> m_nodeVariables;
    std::unordered_map<Edge*, IloNumVar> m_edgeVariables;
    std::unordered_map<llvm::Function*, int> m_functionIdx;
    Partition::FunctionSet m_movedFunctions;
}; // class Impl

ILPOptimization::Impl::Impl(const CallGraph& callgraph,
                            Partition& securePartition,
                            Partition& insecurePartition,
                            Logger& logger)
    : m_callgraph(callgraph)
    , m_securePartition(securePartition)
    , m_insecurePartition(insecurePartition)
    , m_logger(logger)
    , m_ilpModel(m_ilpEnv)
    , m_cplex(m_ilpModel)
{
}

void ILPOptimization::Impl::run()
{
    createNodeVariables();
    createEdgeVariables();
    createConstraints();
    createObjective();
    m_cplex.exportModel("partition_model.lp");

    if ( !m_cplex.solve() ) {
        m_ilpEnv.error() << "Failed to optimize LP" << endl;
        return;
    }
    IloNumArray vals(m_ilpEnv);
    m_ilpEnv.out() << "Solution status " << m_cplex.getStatus() << std::endl;
    for (const auto& [node, var] : m_nodeVariables) {
        if (m_cplex.getValue(var) == 1) {
            m_movedFunctions.insert(node->getFunction());
        }
    }
}

void ILPOptimization::Impl::apply()
{
    for (auto* F : m_movedFunctions) {
        m_securePartition.addToPartition(F);
        m_securePartition.removeRelatedFunction(F);
        m_insecurePartition.removeFromPartition(F);
    }
}

void ILPOptimization::Impl::createNodeVariables()
{
    int i = 0;
    for (auto it = m_callgraph.begin(); it != m_callgraph.end(); ++it) {
        auto [pos, succ] = m_nodeVariables.insert(std::make_pair(it->second.get(),
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
        if (m_securePartition.contains(F)) {
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
    for (const auto& [edge, var] : m_edgeVariables) {
        const auto& edgeCost = edge->getWeight().getValue();
        obj.setLinearCoef(var, edgeCost);
    }
    for (const auto& [node, var] : m_nodeVariables) {
        Double sensitiveRelatedCost;
        Double sizeCost;
        if (node->getWeight().hasFactor(WeightFactor::SENSITIVE_RELATED)) {
            sensitiveRelatedCost = node->getWeight().getFactor(WeightFactor::SENSITIVE_RELATED).getWeight();
        }
        if (node->getWeight().hasFactor(WeightFactor::SIZE)) {
            sizeCost =  node->getWeight().getFactor(WeightFactor::SIZE).getWeight();
        }
        const auto& nodeCost = sensitiveRelatedCost - sizeCost;
        obj.setLinearCoef(var, nodeCost);
    }
    m_ilpModel.add(obj);
}

ILPOptimization::ILPOptimization(const CallGraph& callgraph,
                                 Partition& securePartition,
                                 Partition& insecurePartition,
                                 Logger& logger)
    : PartitionOptimization(securePartition, nullptr, logger, PartitionOptimizer::ILP)
    , m_impl(new Impl(callgraph, securePartition, insecurePartition, logger))
{
}

void ILPOptimization::run()
{
    m_impl->run();
}

void ILPOptimization::apply()
{
    m_impl->apply();
}

} // namesapce vazgen

