#include "Logger.h"

#include "SVF/PDG/PDGPointerAnalysis.h"
#include "SVF/MSSA/SVFG.h"
#include "SVF/MSSA/SVFGBuilder.h"
#include "SVF/Util/SVFModule.h"

#include "llvm/Pass.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/IR/Module.h"
#include "llvm/PassRegistry.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Support/raw_os_ostream.h"
#include "llvm/Transforms/IPO/PassManagerBuilder.h"

#include "dg/llvm/LLVMDependenceGraphBuilder.h"
#include "dg/llvm/LLVMDependenceGraph.h"
#include "dg/llvm/analysis/DefUse/DefUse.h"
#include "dg/llvm/analysis/PointsTo/PointerAnalysis.h"
#include "dg/llvm/analysis/ReachingDefinitions/ReachingDefinitions.h"
#include "dg/analysis/PointsTo/PointerAnalysisFS.h"
#include "dg/analysis/PointsTo/PointerAnalysisFI.h"
#include "dg/analysis/PointsTo/PointerAnalysisFSInv.h"

#include "boost/graph/adjacency_list.hpp"
#include "boost/graph/graph_traits.hpp"

#include <memory>

namespace debug {

using directed_graph = boost::adjacency_list<boost::vecS, boost::vecS, boost::directedS>;
using vertex_desc = directed_graph::vertex_descriptor;

directed_graph create_graph_from_dg(dg::LLVMDependenceGraph* dg) noexcept
{
    directed_graph graph{};
    std::unordered_map<llvm::Value*, vertex_desc> nodes_mapping; 
    for (auto it = dg->begin(); it != dg->end(); ++it) {
        auto vd = boost::add_vertex(graph);
        nodes_mapping.insert(std::make_pair(it->first, vd));
        for (auto edge_it = it->second->data_begin();
                edge_it != it->second->data_end();
                ++edge_it) {
            vertex_desc dest_vd;
            auto dest_node = nodes_mapping.find((*edge_it)->getValue());
            if (dest_node == nodes_mapping.end()) {
                dest_vd = boost::add_vertex(graph);
                nodes_mapping.insert(std::make_pair((*edge_it)->getValue(), dest_vd));
            } else {
                dest_vd = dest_node->second;
            }
            boost::add_edge(vd, dest_vd, graph);
        }
    }
    return graph;
}

class DgPDGComparePass : public llvm::ModulePass
{
public:
    static char ID;
    DgPDGComparePass()
        : llvm::ModulePass(ID)
    {
    }

    bool runOnModule(llvm::Module& M) override
    {
        SVFModule svfModule(M);
        AndersenWaveDiff* ander = new pdg::PDGAndersenWaveDiff();
        ander->disablePrintStat();
        ander->analyze(svfModule);
        SVFGBuilder memSSA(true);
        SVFG *svfg = memSSA.buildSVFG((BVDataPTAImpl*)ander);

        vazgen::Logger logger("test");
        logger.setLevel(vazgen::Logger::ERR);
        dg::llvmdg::LLVMDependenceGraphOptions options;
        options.PTAOptions.analysisType = dg::LLVMPointerAnalysisOptions::AnalysisType::fi;
        options.RDAOptions.analysisType = dg::analysis::LLVMReachingDefinitionsAnalysisOptions::AnalysisType::dense;
        dg::llvmdg::LLVMDependenceGraphBuilder builder(&M, options);
        auto dg = builder.build();
        auto CFs = dg::getConstructedFunctions();
        for (auto& F : M) {
            if (F.isDeclaration()) {
                continue;
            }
            llvm::dbgs() << "Compare function " << F.getName() << "\n";
            logger.info("Compare function " + F.getName().str() + "\n");
            auto F_dg = CFs.find(&F);
            if (F_dg == CFs.end()) {
                logger.error("No dg built for " + F.getName().str() + "\n");
                continue;
            }
            auto dg_graph = create_graph_from_dg(F_dg->second);
            llvm::dbgs() << "Vertices number " << boost::num_vertices(dg_graph) << "\n";
            llvm::dbgs() << "Edge number " << boost::num_edges(dg_graph) << "\n";
        }
                return false;
    }


}; // class DgPDGCompare

char DgPDGComparePass::ID = 0;
static llvm::RegisterPass<DgPDGComparePass> X("dg-pdg-compare","Check if dg and pdg are isomorphic");

}

