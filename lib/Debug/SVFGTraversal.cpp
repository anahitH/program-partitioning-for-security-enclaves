#include "llvm/Pass.h"

#include "llvm/IR/LegacyPassManager.h"
#include "llvm/IR/Module.h"
#include "llvm/PassRegistry.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Support/GraphWriter.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Transforms/IPO/PassManagerBuilder.h"

#include "SVF/PDG/PDGPointerAnalysis.h"
#include "SVF/MSSA/SVFG.h"
#include "SVF/MSSA/SVFGBuilder.h"
#include "SVF/Util/SVFModule.h"
#include "SVF/MemoryModel/PointerAnalysis.h"
#include "SVF/WPA/Andersen.h"

#include <unordered_set>

namespace debug {

bool hasIncomingEdges(PAGNode* pagNode)
{
    //Addr, Copy, Store, Load, Call, Ret, NormalGep, VariantGep, ThreadFork, ThreadJoin
    if (pagNode->hasIncomingEdges(PAGEdge::Addr)
        || pagNode->hasIncomingEdges(PAGEdge::Copy)
        || pagNode->hasIncomingEdges(PAGEdge::Store)
        || pagNode->hasIncomingEdges(PAGEdge::Load)
        || pagNode->hasIncomingEdges(PAGEdge::Call)
        || pagNode->hasIncomingEdges(PAGEdge::Ret)
        || pagNode->hasIncomingEdges(PAGEdge::NormalGep)
        || pagNode->hasIncomingEdges(PAGEdge::VariantGep)
        || pagNode->hasIncomingEdges(PAGEdge::ThreadFork)
        || pagNode->hasIncomingEdges(PAGEdge::ThreadJoin)) {
        return true;
    }
    return false;
}

bool hasOutgoingEdges(PAGNode* pagNode)
{
    //Addr, Copy, Store, Load, Call, Ret, NormalGep, VariantGep, ThreadFork, ThreadJoin
    if (pagNode->hasOutgoingEdges(PAGEdge::Addr)
        || pagNode->hasOutgoingEdges(PAGEdge::Copy)
        || pagNode->hasOutgoingEdges(PAGEdge::Store)
        || pagNode->hasOutgoingEdges(PAGEdge::Load)
        || pagNode->hasOutgoingEdges(PAGEdge::Call)
        || pagNode->hasOutgoingEdges(PAGEdge::Ret)
        || pagNode->hasOutgoingEdges(PAGEdge::NormalGep)
        || pagNode->hasOutgoingEdges(PAGEdge::VariantGep)
        || pagNode->hasOutgoingEdges(PAGEdge::ThreadFork)
        || pagNode->hasOutgoingEdges(PAGEdge::ThreadJoin)) {
        return true;
    }
    return false;
}


class SVFGTraversal : public llvm::ModulePass
{
public:
    static char ID;
    SVFGTraversal()
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
        auto* pag = svfg->getPAG();
        for (auto& F : M) {
            if (F.isDeclaration()) {
                continue;
            }
            llvm::dbgs() << "Function   " << F.getName() << "\n";
            if (svfg->hasFormalOUTSVFGNodes(&F)) {
                llvm::dbgs() << "has formal out nodes\n";
                for (const auto& out : svfg->getFormalOUTSVFGNodes(&F)) {
                    llvm::dbgs() << out << "\n";
                    if (svfg->hasSVFGNode(out)) {
                        llvm::dbgs() << "svfg formal out " << *svfg->getSVFGNode(out) << "\n";
                    } else if (pag->hasGNode(out)) {
                        llvm::dbgs() << "pag formal out " << *pag->getPAGNode(out) << "\n";
                    }
                }
            }
            if (svfg->hasFormalINSVFGNodes(&F)) {
                llvm::dbgs() << "has formal in nodes\n";
                for (const auto& in : svfg->getFormalINSVFGNodes(&F)) {
                    llvm::dbgs() << in << "\n";
                    if (svfg->hasSVFGNode(in)) {
                        llvm::dbgs() << "svfg formal in " << *svfg->getSVFGNode(in) << "\n";
                    } else if (pag->hasGNode(in)) {
                        llvm::dbgs() << "pag formal in " << *pag->getPAGNode(in) << "\n";
                    }
                }
            }
            for (auto arg_it = F.arg_begin(); arg_it != F.arg_end(); ++arg_it) {
                llvm::dbgs() << "Argument " << *arg_it << "\n";
                process(svfg, &*arg_it);
                llvm::dbgs() << "---------------\n";
            }
            for (auto& B : F) {
                for (auto& I : B) {
                    llvm::dbgs() << "INSTR: " << I << "\n";
                    process(svfg, &I);
                    llvm::dbgs() << "---------------\n";
                }
            }
        }
        return false;
    }

    void process(SVFG* svfg, llvm::Value* I)
    {
        auto* pag = svfg->getPAG();
        if (!pag->hasValueNode(I)) {
            llvm::dbgs() << "   No PAG node\n";
            return;
        }
        auto nodeId = pag->getValueNode(I);
        auto* pagNode = pag->getPAGNode(nodeId);
        llvm::dbgs() << "   PAG Node " << *pagNode << "\n";
        if (!hasIncomingEdges(pagNode) && !hasOutgoingEdges(pagNode)) {
            llvm::dbgs() << "   No incoming or outgoing edges\n";
            return;
        }
        processPAGNode(pagNode, svfg);
        if (auto* callInst = llvm::dyn_cast<llvm::CallInst>(I)) {
            llvm::CallSite callSite(callInst);
            if (svfg->hasActualOUTSVFGNodes(callSite)) {
                llvm::dbgs() << "   Has actual out svfg nodes\n";
                const auto& actualOutNodes = svfg->getActualOUTSVFGNodes(callSite);
                for (const auto& actualOut : actualOutNodes) {
                    llvm::dbgs() << actualOut << "\n";
                    if (svfg->hasSVFGNode(actualOut)) {
                        llvm::dbgs() << "svfg formal out " << *svfg->getSVFGNode(actualOut) << "\n";
                    } else if (pag->hasGNode(actualOut)) {
                        llvm::dbgs() << "pag formal out " << *pag->getPAGNode(actualOut) << "\n";
                    }

                }
            }
            if (svfg->hasActualINSVFGNodes(callSite)) {
                llvm::dbgs() << "   Has actual in svfg nodes\n";
                const auto& actualInNodes = svfg->getActualINSVFGNodes(callSite);
                for (const auto& actualIn : actualInNodes) {
                    llvm::dbgs() << actualIn << "\n";
                    if (svfg->hasSVFGNode(actualIn)) {
                        llvm::dbgs() << "svfg formal in " << *svfg->getSVFGNode(actualIn) << "\n";
                    } else if (pag->hasGNode(actualIn)) {
                        llvm::dbgs() << "pag formal in " << *pag->getPAGNode(actualIn) << "\n";
                    }

                }
            }
        }
        // else if (auto* mssaPhi = llvm::dyn_cast<MSSAPHISVFGNode>(svfgNode)) {
                   // }
    }

    void processPAGNode(PAGNode* node, SVFG* svfg)
    {
        if (!svfg->hasDef(node)) {
            return;
        }
        auto* svfgNode = svfg->getDefSVFGNode(node);
        llvm::dbgs() << "   SVFG node " << *svfgNode << "\n";
        processSVFGNode(const_cast<SVFGNode*>(svfgNode), svfg);
        printEdges(svfgNode, svfg);
    }

    void processSVFGNode(SVFGNode* svfgNode, SVFG* svfg)
    {
        if (auto* stmtNode = llvm::dyn_cast<StmtSVFGNode>(svfgNode)) {
            processStmtNode(const_cast<StmtSVFGNode*>(stmtNode), svfg);
        } else if (auto* actualParamNode = llvm::dyn_cast<ActualParmSVFGNode>(svfgNode)) {
            processActualParamNode(const_cast<ActualParmSVFGNode*>(actualParamNode), svfg);
        } else if (auto* actualRet = llvm::dyn_cast<ActualRetSVFGNode>(svfgNode)) {
            processActualRetNode(const_cast<ActualRetSVFGNode*>(actualRet), svfg);
        } else if (auto* formalParam = llvm::dyn_cast<FormalParmSVFGNode>(svfgNode)) {
            processFormalParamNode(const_cast<FormalParmSVFGNode*>(formalParam), svfg);
        } else  if (auto* formalRet = llvm::dyn_cast<FormalRetSVFGNode>(svfgNode)) {
            processFormalRetNode(const_cast<FormalRetSVFGNode*>(formalRet), svfg);
        } else if (auto* formalInNode = llvm::dyn_cast<FormalINSVFGNode>(svfgNode)) {
            processFormalInNode(const_cast<FormalINSVFGNode*>(formalInNode), svfg);
        } else if (auto* formalOutNode = llvm::dyn_cast<FormalOUTSVFGNode>(svfgNode)) {
            processFormalOutNode(const_cast<FormalOUTSVFGNode*>(formalOutNode), svfg);
        } else if (auto* actualInNode = llvm::dyn_cast<ActualINSVFGNode>(svfgNode)) {
            processActualInNode(const_cast<ActualINSVFGNode*>(actualInNode), svfg);
        } else if (auto* actualOutNode = llvm::dyn_cast<ActualOUTSVFGNode>(svfgNode)) {
            processActualOutNode(const_cast<ActualOUTSVFGNode*>(actualOutNode), svfg);
        } else if (auto* intraMssaPhiNode = llvm::dyn_cast<IntraMSSAPHISVFGNode>(svfgNode)) {
            processIntraMssaPhiNode(const_cast<IntraMSSAPHISVFGNode*>(intraMssaPhiNode), svfg);
        } else if (auto* interMssaPhiNode = llvm::dyn_cast<InterMSSAPHISVFGNode>(svfgNode)) {
            processInterMssaPhiNode(const_cast<InterMSSAPHISVFGNode*>(interMssaPhiNode), svfg);
        } else if (auto* null = llvm::dyn_cast<NullPtrSVFGNode>(svfgNode)) {
            llvm::dbgs() << "       Null Node\n";
        } else if (auto* intraPhiNode = llvm::dyn_cast<IntraPHISVFGNode>(svfgNode)) {
            processIntraPhiNode(const_cast<IntraPHISVFGNode*>(intraPhiNode), svfg);
        } else if (auto* interPhiNode = llvm::dyn_cast<InterPHISVFGNode>(svfgNode)) {
            processInterPhiNode(const_cast<InterPHISVFGNode*>(interPhiNode), svfg);
        }
    }

    void printEdges(const SVFGNode* svfgNode, SVFG* svfg)
    {
        llvm::dbgs() << "INCOMING EDGES\n";
        for (auto inedge_it = svfgNode->InEdgeBegin(); inedge_it != svfgNode->InEdgeEnd(); ++inedge_it) {
            printEdge(*inedge_it, svfg, true);
        }
        llvm::dbgs() << "OUT EDGES\n";
        for (auto edge_it = svfgNode->OutEdgeBegin(); edge_it != svfgNode->OutEdgeEnd(); ++edge_it) {
            printEdge(*edge_it, svfg, false);
        }
    }

    void printEdge(SVFGEdge* edge, SVFG* svfg, bool incoming)
    {
        llvm::dbgs() << "   Edge type ";
        auto edgeKind = edge->getEdgeKind();
        if (edgeKind == SVFGEdge::IntraDirect) {
            llvm::dbgs() << "IntraDirect\n";
        } else if (edgeKind == SVFGEdge::IntraIndirect) {
            llvm::dbgs() << "IntraIndirect\n";
        } else if (edgeKind == SVFGEdge::DirCall) {
            llvm::dbgs() << "DirCall\n";
        } else if (edgeKind == SVFGEdge::DirRet) {
            llvm::dbgs() << "DirRet\n";
        } else if (edgeKind == SVFGEdge::IndCall) {
            llvm::dbgs() << "IndCall\n";
        } else if (edgeKind == SVFGEdge::IndRet) {
            llvm::dbgs() << "IndRet\n";
        } else if (edgeKind == SVFGEdge::TheadMHPIndirect) {
            llvm::dbgs() << "TheadMHPIndirect\n";
        }
        SVFGNode* node = nullptr;
        if (incoming) {
            node = edge->getSrcNode();
            llvm::dbgs() << "       Edge Src node\n";
        } else {
            node = edge->getDstNode();
            llvm::dbgs() << "       Edge Dst node\n";
        }
        processSVFGNode(const_cast<SVFGNode*>(node), svfg);

    }

    void processStmtNode(StmtSVFGNode* stmtNode, SVFG* svfg)
    {
        if (stmtNode->getInst()) {
            llvm::dbgs() << "       Stmt Node " << *stmtNode->getInst() << "\n";
        }
        SVFGNode* defNode = nullptr;
        if (auto* addrNode = llvm::dyn_cast<AddrSVFGNode>(stmtNode)) {
            if (svfg->hasDef(addrNode->getPAGSrcNode())) {
                defNode = const_cast<SVFGNode*>(svfg->getDefSVFGNode(addrNode->getPAGSrcNode()));
            }
        } else if (auto* cpyNode = llvm::dyn_cast<CopySVFGNode>(stmtNode)) {
            if (svfg->hasDef(cpyNode->getPAGSrcNode())) {
                defNode = const_cast<SVFGNode*>(svfg->getDefSVFGNode(cpyNode->getPAGSrcNode()));
            }
        } else if (auto* gepNode = llvm::dyn_cast<GepSVFGNode>(stmtNode)) {
            if (svfg->hasDef(gepNode->getPAGSrcNode())) {
                defNode = const_cast<SVFGNode*>(svfg->getDefSVFGNode(gepNode->getPAGSrcNode()));
            }
        } else if (auto* loadNode = llvm::dyn_cast<LoadSVFGNode>(stmtNode)) {
            if (svfg->hasDef(loadNode->getPAGSrcNode())) {
                defNode = const_cast<SVFGNode*>(svfg->getDefSVFGNode(loadNode->getPAGSrcNode()));
            }
        } else if (auto* storeNode = llvm::dyn_cast<StoreSVFGNode>(stmtNode)) {
            llvm::dbgs() << "Store node\n";
        }
        if (defNode) {
            llvm::dbgs() << "Def node: " << *defNode << "\n";
        }
    }

    void processActualParamNode(ActualParmSVFGNode* actualParamNode, SVFG* svfg)
    {
        llvm::dbgs() << "       Actual param node\n";
        llvm::dbgs() << "       Call site " << *(actualParamNode->getCallSite().getInstruction()) << "\n";
        llvm::dbgs() << "       Param " << *actualParamNode->getParam() << "\n";
        auto* pagNode = actualParamNode->getParam();
        processPAGNode(const_cast<PAGNode*>(pagNode), svfg);
    }

    void processActualRetNode(ActualRetSVFGNode* actualRetNode, SVFG* svfg)
    {
        llvm::dbgs() << "       Actual ret node\n";
        llvm::dbgs() << "       Call site " << *(actualRetNode->getCallSite().getInstruction()) << "\n";
        llvm::dbgs() << "       Rev " << *actualRetNode->getRev() << "\n";
        auto* pagNode = actualRetNode->getRev();
        processPAGNode(const_cast<PAGNode*>(pagNode), svfg);
    }

    void processFormalParamNode(FormalParmSVFGNode* formalParamNode, SVFG* svfg)
    {
        llvm::dbgs() << "       Formal param node\n";
        llvm::dbgs() << "       Function " << formalParamNode->getFun()->getName() << "\n";
        llvm::dbgs() << "       Param " << *formalParamNode->getParam() << "\n";
        for (auto it = formalParamNode->callPEBegin(); it != formalParamNode->callPEEnd(); ++it) {
            llvm::dbgs() << "       callPE callSite " << *((*it)->getCallInst()) << "\n";
            llvm::dbgs() << "       source node " << *(*it)->getSrcNode() << "\n";
            llvm::dbgs() << "       dest node " << *(*it)->getDstNode() << "\n";
            processPAGNode(const_cast<PAGNode*>((*it)->getSrcNode()), svfg);
        }
    }

    void processFormalRetNode(FormalRetSVFGNode* formalRetNode, SVFG* svfg)
    {
        llvm::dbgs() << "       Formal ret node\n";
        llvm::dbgs() << "       Function " << formalRetNode->getFun()->getName() << "\n";
        llvm::dbgs() << "       Ret " << *formalRetNode->getRet() << "\n";
        for (auto it = formalRetNode->retPEBegin(); it != formalRetNode->retPEEnd(); ++it) {
            llvm::dbgs() << "       retPE callSite " << *(*it)->getCallInst() << "\n";
            llvm::dbgs() << "       source node " << *(*it)->getSrcNode() << "\n";
            llvm::dbgs() << "       dest node " << *(*it)->getDstNode() << "\n";
        }
        // No def for FormalRet node
    }

    void processFormalInNode(FormalINSVFGNode* formalInNode, SVFG* svfg)
    {
        llvm::dbgs() << "       Formal IN node\n";
        llvm::dbgs() << "       Entry CHI ";
        const_cast<EntryCHI<DdNode*>*>(formalInNode->getEntryChi())->dump();
        llvm::dbgs() << "       Res Ver def \n";
        const_cast< MSSADEF*>(formalInNode->getEntryChi()->getResVer()->getDef())->dump();
        llvm::dbgs() << "       Res Ver mem region \n";
        formalInNode->getEntryChi()->getResVer()->getMR()->dumpStr();
        llvm::dbgs() << "       Op Ver def\n";
        const_cast< MSSADEF*>(formalInNode->getEntryChi()->getOpVer()->getDef())->dump();
        llvm::dbgs() << "       Op Ver mem region \n";
        formalInNode->getEntryChi()->getOpVer()->getMR()->dumpStr();
    }

    void processFormalOutNode(FormalOUTSVFGNode* formalOutNode, SVFG* svfg)
    {
        llvm::dbgs() << "       Formal OUT node\n";
        llvm::dbgs() << "       Ret MU \n";
        const_cast<RetMU<DdNode*>*>(formalOutNode->getRetMU())->dump();
        // Has PointsTo
    }

    void processActualInNode(ActualINSVFGNode* actualInNode, SVFG* svfg)
    {
        llvm::dbgs() << "       Actual IN node\n";
        llvm::dbgs() << "       Call MU \n";
        const_cast<CallMU<DdNode*>*>(actualInNode->getCallMU())->dump();
        // Has PointsTo
    }

    void processActualOutNode(ActualOUTSVFGNode* actualOutNode, SVFG* svfg)
    {
        llvm::dbgs() << "       Actual OUT node\n";
        llvm::dbgs() << "       Call CHI \n";
        const_cast<CallCHI<DdNode*>*>(actualOutNode->getCallCHI());
        llvm::dbgs() << "       Op Ver def\n";
        const_cast< MSSADEF*>(actualOutNode->getCallCHI()->getOpVer()->getDef())->dump();
        llvm::dbgs() << "       Op Ver mem region \n";
        actualOutNode->getCallCHI()->getMR()->dumpStr();
    }

    void processIntraMssaPhiNode(IntraMSSAPHISVFGNode* intraMssaPhiNode, SVFG* svfg)
    {
        std::unordered_set<MSSADEF*> defs;
        auto* res_def = const_cast< MSSADEF*>(intraMssaPhiNode->getRes());
        if (!defs.insert(res_def).second) {
            return;
        }
        llvm::dbgs() << "       Intra MSSA phi node\n";
        llvm::dbgs() << "       Res \n";
        res_def->dump();
        llvm::dbgs() << intraMssaPhiNode->getOpVerNum() << "\n";
        for (auto it = intraMssaPhiNode->opVerBegin(); it != intraMssaPhiNode->opVerEnd(); ++it) {
            auto* def = it->second->getDef();
            llvm::dbgs() << "       op ver def \n";
            processMSSADef(def, defs);
            //const_cast< MSSADEF*>(def)->dump();
            //llvm::dbgs() << "       Op Ver mem region " << def->getMR()->dumpStr() << "\n";
        }
    }

    void processMSSADef(MSSADEF* def, std::unordered_set<MSSADEF*>& defs)
    {
        if (!defs.insert(def).second) {
            return;
        }
        if (def->getType() == MSSADEF::CallMSSACHI) {
            auto* callChi = llvm::dyn_cast<SVFG::CALLCHI>(def);
            llvm::dbgs() << "           Call CHI ";
            callChi->dump();
        } else if (def->getType() == MSSADEF::StoreMSSACHI) {
            auto* storeChi = llvm::dyn_cast<SVFG::STORECHI>(def);
            llvm::dbgs() << "           Store CHI " << *storeChi->getStoreInst()->getInst() << "\n";
        } else if (def->getType() == MSSADEF::EntryMSSACHI) {
            llvm::dbgs() << "           Entry Chi\n";
        } else if (def->getType() == MSSADEF::SSAPHI) {
            auto* phi = llvm::dyn_cast<MemSSA::PHI>(def);
            llvm::dbgs() << "           Phi\n";
            for (auto it = phi->opVerBegin(); it != phi->opVerEnd(); ++it) {
                processMSSADef(it->second->getDef(), defs);
            }
        }
    }

    void processInterMssaPhiNode(InterMSSAPHISVFGNode* interMssaPhiNode, SVFG* svfg)
    {
        llvm::dbgs() << "       Inter MSSA phi node\n";
        llvm::dbgs() << "       Res\n";
        const_cast< MSSADEF*>(interMssaPhiNode->getRes())->dump();
        for (auto it = interMssaPhiNode->opVerBegin(); it != interMssaPhiNode->opVerEnd(); ++it) {
            llvm::dbgs() << "       op ver def \n";
            const_cast< MSSADEF*>(it->second->getDef())->dump();
            llvm::dbgs() << "       Op Ver mem region \n";
            it->second->getMR()->dumpStr();
        }
    }

    void processIntraPhiNode(IntraPHISVFGNode* intraPhiNode, SVFG* svfg)
    {
        llvm::dbgs() << "       Intra PHI node\n";
        llvm::dbgs() << "       Res " << *intraPhiNode->getRes() << "\n";
        for (auto it = intraPhiNode->opVerBegin(); it != intraPhiNode->opVerEnd(); ++it) {
            llvm::dbgs() << "       op ver: " << *it->second << "\n";
        }
    }

    void processInterPhiNode(InterPHISVFGNode* interPhiNode, SVFG* svfg)
    {
        llvm::dbgs() << "   Inter PHI node\n";
        llvm::dbgs() << "       Res " << *interPhiNode->getRes() << "\n";
        for (auto it = interPhiNode->opVerBegin(); it != interPhiNode->opVerEnd(); ++it) {
            llvm::dbgs() << "       op ver: " << *it->second << "\n";
        }
    }
};

char SVFGTraversal::ID = 0;
static llvm::RegisterPass<SVFGTraversal> X("svfg-traversal","Traverse SVFG graph and print information");

} // namespace debug

