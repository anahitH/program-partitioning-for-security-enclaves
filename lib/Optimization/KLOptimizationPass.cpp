#include "Optimization/KLOptimizationPass.h"

#include "Analysis/CallGraph.h"
#include "Analysis/Partition.h"
#include "Utils/Logger.h"

#include "llvm/IR/Function.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/raw_ostream.h"

#include <algorithm>

namespace vazgen {

class KLOptimizationPass::Impl
{
public:
    Impl(const CallGraph& callgraph,
         Partition& securePartition,
         Partition& insecurePartition,
         Logger& logger);

public:
    void setCandidates(const Functions& candidates);
    void run();

private:
    void computeCandidateEdgeCosts();
    void computeMoveGains(llvm::Function* movedF);
    void computeInitialMoveGains();
    std::pair<Double, Double> getFunctionCosts(Node* node);
    void moveFunction(int idx);
    void applyOptimization();

private:
    const CallGraph& m_callgraph;
    Partition& m_securePartition;
    Partition& m_insecurePartition;
    Logger& m_logger;
    Functions m_candidates;
    std::vector<Double> m_moveGains;
    // for each function the gain when it's moved
    std::vector<std::pair<llvm::Function*, Double>> m_functionMoveGains;
    // For each function its' edge cost with the rest of functions
    std::unordered_map<llvm::Function*, std::unordered_map<llvm::Function*, Double>> m_functionEdgeCosts;
}; // class KLOptimizationPass::Impl

KLOptimizationPass::Impl::Impl(const CallGraph& callgraph,
                               Partition& securePartition,
                               Partition& insecurePartition,
                               Logger& logger)
    : m_callgraph(callgraph)
    , m_securePartition(securePartition)
    , m_insecurePartition(insecurePartition)
    , m_logger(logger)
{
}

void KLOptimizationPass::Impl::setCandidates(const Functions& candidates)
{
    m_candidates = candidates;
}

void KLOptimizationPass::Impl::run()
{
    computeCandidateEdgeCosts();
    llvm::Function* movedF = nullptr;
    while (!m_candidates.empty()) {
        computeMoveGains(movedF);
        auto maxGainPos = std::max_element(m_moveGains.begin(), m_moveGains.end());
        int maxGainIdx = std::distance(m_moveGains.begin(), maxGainPos);
        movedF = m_candidates[maxGainIdx];
        moveFunction(maxGainIdx);
    }
    applyOptimization();
}

void KLOptimizationPass::Impl::computeCandidateEdgeCosts()
{
    for (auto F : m_candidates) {
        if (!m_callgraph.hasFunctionNode(F)) {
            continue;
        }
        auto& edgeCosts = m_functionEdgeCosts[F];
        auto* Fnode = m_callgraph.getFunctionNode(F);
        for (auto it = Fnode->inEdgesBegin(); it != Fnode->inEdgesEnd(); ++it) {
            auto weight = it->getWeight().getFactor(WeightFactor::CALL_NUM).getWeight();
            edgeCosts[it->getSource()->getFunction()] += weight;
        }
        for (auto it = Fnode->outEdgesBegin(); it != Fnode->outEdgesEnd(); ++it) {
            auto weight = it->getWeight().getFactor(WeightFactor::CALL_NUM).getWeight();
            edgeCosts[it->getSink()->getFunction()] += weight;
        }
    }
}

void KLOptimizationPass::Impl::computeMoveGains(llvm::Function* movedF)
{
    if (!movedF) {
        m_moveGains.clear();
        m_moveGains.resize(m_candidates.size());
        computeInitialMoveGains();
        return;
    }
    for (int i = 0; i < m_candidates.size(); ++i) {
        auto edgeCost = m_functionEdgeCosts[m_candidates[i]][movedF];
        m_moveGains[i] += 2 * m_functionEdgeCosts[m_candidates[i]][movedF];
    }
}

void KLOptimizationPass::Impl::computeInitialMoveGains()
{
    for (int i = 0; i < m_candidates.size(); ++i) {
        llvm::Function* F = m_candidates[i];
        assert(m_callgraph.hasFunctionNode(F));
        auto* Fnode = m_callgraph.getFunctionNode(F);
        const auto& costs = getFunctionCosts(Fnode);
        m_moveGains[i] = costs.second - costs.first;
    }
}

std::pair<Double, Double> KLOptimizationPass::Impl::getFunctionCosts(Node* node)
{
    Double internalCost = 0;
    Double externalCost = 0;
    for (auto it = node->inEdgesBegin(); it != node->inEdgesEnd(); ++it) {
        llvm::Function* source = it->getSource()->getFunction();
        auto weight = it->getWeight().getFactor(WeightFactor::CALL_NUM).getWeight();
        if (m_insecurePartition.contains(source)) {
            internalCost += weight;
        } else {
            externalCost += weight;
        }
    }
    for (auto it = node->outEdgesBegin(); it != node->outEdgesEnd(); ++it) {
        llvm::Function* sink = it->getSink()->getFunction();
        auto weight = it->getWeight().getFactor(WeightFactor::CALL_NUM).getWeight();
        if (m_insecurePartition.contains(sink)) {
            internalCost += weight;
        } else {
            externalCost += weight;
        }
    }
    return std::make_pair(internalCost, externalCost);
}

void KLOptimizationPass::Impl::moveFunction(int idx)
{
    llvm::Function* F = m_candidates[idx];
    auto gain = m_moveGains[idx];
    m_functionMoveGains.push_back(std::make_pair(F, gain));
    m_candidates.erase(m_candidates.begin() + idx);
    m_moveGains.erase(m_moveGains.begin() + idx);
    m_securePartition.addToPartition(F);
    m_insecurePartition.removeFromPartition(F);
}

void KLOptimizationPass::Impl::applyOptimization()
{
    Double maxGain = 0;
    Double intmdGain = 0;
    int idx = -1;
    for (int i = 0; i < m_functionMoveGains.size(); ++i) {
        intmdGain += m_functionMoveGains[i].second;
        if (maxGain <= intmdGain) {
            maxGain = intmdGain;
            idx = i;
        }
    }
    for (int i = idx + 1; i <= m_functionMoveGains.size(); ++i) {
        llvm::Function* revertF = m_functionMoveGains[i].first;
        m_securePartition.removeFromPartition(revertF);
        m_insecurePartition.addToPartition(revertF);
    }
}

KLOptimizationPass::KLOptimizationPass(const CallGraph& callgraph,
                                       Partition& securePartition,
                                       Partition& insecurePartition,
                                       Logger& logger)
    : m_impl(new Impl(callgraph, securePartition, insecurePartition, logger))
{
}

void KLOptimizationPass::setCandidates(const Functions& candidates)
{
    m_impl->setCandidates(candidates);
}

void KLOptimizationPass::run()
{
    m_impl->run();
}

} // namespace vazgen

