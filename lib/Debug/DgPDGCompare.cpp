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
#include "llvm/IR/Metadata.h"

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
#include <boost/graph/isomorphism.hpp>

#include <memory>

namespace debug {

using directed_graph = boost::adjacency_list<boost::vecS, boost::vecS, boost::directedS>;
using vertex_desc = directed_graph::vertex_descriptor;

void add_edge(directed_graph& graph, llvm::Value* src,
              llvm::Value* dst,
              std::unordered_map<llvm::Value*, vertex_desc>& nodes_mapping)
{
    if (src == dst) {
        return;
    }
    if (!dst || !src) {
        return;
    }
    if (llvm::isa<llvm::Constant>(src)
            || llvm::isa<llvm::Constant>(dst)
            || llvm::isa<llvm::MetadataAsValue>(src)
            || llvm::isa<llvm::MetadataAsValue>(dst)
            || llvm::isa<llvm::BasicBlock>(src)
            || llvm::isa<llvm::BasicBlock>(dst)) {
        return;
    }
    auto* src_inst = llvm::dyn_cast<llvm::Instruction>(src);
    auto* dst_inst = llvm::dyn_cast<llvm::Instruction>(dst);
    if (src_inst && dst_inst && src_inst->getFunction() != dst_inst->getFunction()) {
        return;
    }
    vertex_desc src_vd;
    vertex_desc dst_vd;
    auto src_node = nodes_mapping.find(const_cast<llvm::Value*>(src));
    if (src_node == nodes_mapping.end()) {
        src_vd = boost::add_vertex(graph);
        nodes_mapping.insert(std::make_pair(const_cast<llvm::Value*>(src), src_vd));
    } else {
        src_vd = src_node->second;
    }
    auto dst_node = nodes_mapping.find(const_cast<llvm::Value*>(dst));
    if (dst_node == nodes_mapping.end()) {
        dst_vd = boost::add_vertex(graph);
        nodes_mapping.insert(std::make_pair(const_cast<llvm::Value*>(dst), dst_vd));
    } else {
        dst_vd = dst_node->second;
    }

    if (!boost::edge(src_vd, dst_vd, graph).second) {
        boost::add_edge(src_vd, dst_vd, graph);
        //llvm::dbgs() << "Edge from " << *src << " to " << *dst << "\n";
    }
}

directed_graph create_graph_from_dg(dg::LLVMDependenceGraph* dg, llvm::Function* F) noexcept
{
    directed_graph graph{};
    std::unordered_map<llvm::Value*, vertex_desc> nodes_mapping; 
    for (auto it = dg->begin(); it != dg->end(); ++it) {
        for (auto edge_it = it->second->data_begin();
                edge_it != it->second->data_end();
                ++edge_it) {
            add_edge(graph, it->first, (*edge_it)->getValue(), nodes_mapping);
        }
        for (auto edge_it = it->second->use_begin();
                edge_it != it->second->use_end();
                ++edge_it) {
            add_edge(graph, it->first, (*edge_it)->getValue(), nodes_mapping);
        }
    }
    for (auto arg_it = F->arg_begin(); arg_it != F->arg_end(); ++arg_it) {
        auto* arg_node = dg->getNode(&*arg_it);
        for (auto edge_it = arg_node->data_begin(); edge_it != arg_node->data_end(); ++edge_it) {
            add_edge(graph, arg_node->getValue(), (*edge_it)->getValue(), nodes_mapping);
        }
        for (auto edge_it = arg_node->use_begin(); edge_it != arg_node->use_end(); ++edge_it) {
            add_edge(graph, arg_node->getValue(), (*edge_it)->getValue(), nodes_mapping);
        }
    }
    return graph;
}

void get_destination_values_from_mssa_def(MSSADEF* def,
                                          std::unordered_set<llvm::Value*>& dstInstructions,
                                          std::unordered_set<MSSADEF*>& processed_defs)
{
    if (!processed_defs.insert(def).second) {
        return;
    }
    if (def->getType() == MSSADEF::CallMSSACHI) {
        auto* callChi = llvm::dyn_cast<SVFG::CALLCHI>(def);
        dstInstructions.insert(callChi->getCallSite().getInstruction());
    } else if (def->getType() == MSSADEF::StoreMSSACHI) {
        auto* storeChi = llvm::dyn_cast<SVFG::STORECHI>(def);
        dstInstructions.insert(const_cast<llvm::Instruction*>(storeChi->getStoreInst()->getInst()));
    } else if (def->getType() == MSSADEF::EntryMSSACHI) {
    } else if (def->getType() == MSSADEF::SSAPHI) {
        auto* phi = llvm::dyn_cast<MemSSA::PHI>(def);
        for (auto it = phi->opVerBegin(); it != phi->opVerEnd(); ++it) {
            get_destination_values_from_mssa_def(it->second->getDef(), dstInstructions, processed_defs);
        }
    }
}

std::unordered_set<llvm::Value*> get_destination_values(SVFG* svfg, SVFGNode* node)
{
    std::unordered_set<llvm::Value*> dstInstructions;
    if (auto* dstStmtNode = llvm::dyn_cast<StmtSVFGNode>(node)) {
        auto* dstInst = dstStmtNode->getInst();
        dstInstructions.insert(const_cast<llvm::Instruction*>(dstInst));
    } else if (auto* intraMssaPhiNode = llvm::dyn_cast<IntraMSSAPHISVFGNode>(node)) {
        for (auto it = intraMssaPhiNode->opVerBegin(); it != intraMssaPhiNode->opVerEnd(); ++it) {
            auto* def = it->second->getDef();
            std::unordered_set<MSSADEF*> defs;
            get_destination_values_from_mssa_def(def, dstInstructions, defs);
        }
    } else if (auto* interMssaPhiNode = llvm::dyn_cast<InterMSSAPHISVFGNode>(node)) {
        // skip inter for now
    } else if (auto* intraPhiNode = llvm::dyn_cast<IntraPHISVFGNode>(node)) {
        for (auto it = intraPhiNode->opVerBegin(); it != intraPhiNode->opVerEnd(); ++it) {
            if (it->second->hasValue()) {
                dstInstructions.insert(const_cast<llvm::Value*>(it->second->getValue()));
            }
        }
    } else if (auto* interPhiNode = llvm::dyn_cast<InterPHISVFGNode>(node)) {
        if (interPhiNode->isActualRetPHI()) {
            dstInstructions.insert(const_cast<llvm::Instruction*>(interPhiNode->getCallSite().getInstruction()));
        }
    }
    return dstInstructions;
}

directed_graph create_graph_from_pdg(llvm::Function& F, SVFG* svfg)
{
    directed_graph graph{};
    auto pag = svfg->getPAG();
    std::unordered_map<llvm::Value*, vertex_desc> nodes_mapping;
    for (auto& B : F) {
        for (auto& I : B) {
            if (!pag->hasValueNode(&I)) {
                continue;
            }
            //llvm::dbgs() << I << "\n";
            for (auto op_it = I.op_begin(); op_it != I.op_end(); ++op_it) {
                if (auto* val = llvm::dyn_cast<llvm::Value>(*op_it)) {
                    add_edge(graph, val, &I, nodes_mapping);
                }
            }
            auto nodeID = pag->getValueNode(&I);
            auto node = pag->getPAGNode(nodeID);
            if (!svfg->hasDef(node)) {
                continue;
            }
            auto svfgNode = svfg->getDefSVFGNode(node);
            if (auto* stmtNode = llvm::dyn_cast<StmtSVFGNode>(svfgNode)) {
                for (auto edge : stmtNode->getOutEdges()) {
                    auto dstNode = edge->getDstNode();
                    const auto& dstInstructions = get_destination_values(svfg, dstNode);
                    for (auto& dstInst : dstInstructions) {
                        add_edge(graph, &I, dstInst, nodes_mapping);
                    }
                }
                for (auto edge : stmtNode->getInEdges()) {
                    auto srcNode = edge->getSrcNode();
                    const auto& srcInstructions = get_destination_values(svfg, srcNode);
                    for (auto& srcInst : srcInstructions) {
                        add_edge(graph, srcInst, &I, nodes_mapping);
                    }
                }

            }
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
        AndersenWaveDiff* ander = new svfg::PDGAndersenWaveDiff();
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
            llvm::dbgs() << "******* Compare function " << F.getName() << "*********** \n";
            logger.info("Compare function " + F.getName().str() + "\n");
            auto F_dg = CFs.find(&F);
            if (F_dg == CFs.end()) {
                logger.error("No dg built for " + F.getName().str() + "\n");
                continue;
            }
            auto dg_graph = create_graph_from_dg(F_dg->second, &F);
            llvm::dbgs() << "DG Vertices number " << boost::num_vertices(dg_graph) << "\n";
            llvm::dbgs() << "DG Edge number " << boost::num_edges(dg_graph) << "\n";
            auto pdg_graph = create_graph_from_pdg(F, svfg);
            llvm::dbgs() << "PDG Vertices number " << boost::num_vertices(pdg_graph) << "\n";
            llvm::dbgs() << "PDG Edge number " << boost::num_edges(pdg_graph) << "\n";
            if (!boost::isomorphism(dg_graph, pdg_graph)) {
                llvm::dbgs() << "NOT ISOMORPHIC\n\n\n";
            } else {
                llvm::dbgs() << "ISOMORPHIC\n\n\n";
            }

        }
        return false;
    }


}; // class DgPDGCompare

char DgPDGComparePass::ID = 0;
static llvm::RegisterPass<DgPDGComparePass> X("dg-pdg-compare","Check if dg and pdg are isomorphic");

}

