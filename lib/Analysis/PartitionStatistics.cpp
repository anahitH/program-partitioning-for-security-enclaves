#include "Analysis/PartitionStatistics.h"

#include "Analysis/CallGraph.h"
#include "Analysis/Partition.h"
#include "Utils/Utils.h"

#include "llvm/IR/Module.h"
#include "llvm/IR/Function.h"

namespace vazgen {

PartitionStatistics::PartitionStatistics(std::ofstream& strm,
                                         const Partition& partition,
                                         const CallGraph& callgraph,
                                         llvm::Module& M)
    : Statistics(strm, Statistics::JSON)
    , m_partition(partition)
    , m_callgraph(callgraph)
    , m_module(M)
{
}

void PartitionStatistics::report()
{
    reportPartitionFunctions();
    reportSecurityRelatedFunctions();
    //reportInInterface();
    //reportOutInterface();
    //reportGlobals();
    reportNumOfContextSwitches();
    reportSizeOfTCB();
    repotArgsPassedAccrossPartition();
    flush();
}


void PartitionStatistics::reportPartitionFunctions()
{
    std::vector<std::string> partitionFs;
    partitionFs.reserve(m_partition.getPartition().size());
    std::transform(m_partition.getPartition().begin(), m_partition.getPartition().end(), std::back_inserter(partitionFs),
            [] (llvm::Function* F) { return F->getName().str();});
    write_entry({"program_partition", "partition_functions"}, partitionFs);
    write_entry({"program_partition", "partition_size"}, (unsigned) partitionFs.size());
    double partition_portion = (m_partition.getPartition().size() * 100.0) / m_module.size();
    write_entry({"program_partition", "partition%"}, partition_portion);
}

void PartitionStatistics::reportSecurityRelatedFunctions()
{
    std::vector<std::string> staticAnalysisFs;
    staticAnalysisFs.reserve(m_partition.getRelatedFunctions().size());
    std::transform(m_partition.getRelatedFunctions().begin(), m_partition.getRelatedFunctions().end(), std::back_inserter(staticAnalysisFs),
            [] (const auto& pair) { return pair.first->getName().str() + std::to_string(pair.second);});
    write_entry({"program_partition", "security_related_functions"}, staticAnalysisFs);
    write_entry({"program_partition", "security_related_functions_size"}, (unsigned) staticAnalysisFs.size());
    double security_related_portion = (m_partition.getRelatedFunctions().size() * 100.0) / m_module.size();
    write_entry({"program_partition", "security_related%"}, security_related_portion);
}

void PartitionStatistics::reportInInterface()
{
    std::vector<std::string> inInterface;
    inInterface.reserve(m_partition.getInInterface().size());
    std::transform(m_partition.getInInterface().begin(), m_partition.getInInterface().end(), std::back_inserter(inInterface),
            [] (llvm::Function* F) { return F->getName().str();});
    write_entry({"program_partition", "in_interface"}, inInterface);
    write_entry({"program_partition", "in_interface_size"}, (unsigned) inInterface.size());
    double inInterface_portion = (m_partition.getInInterface().size() * 100.0) / m_module.size();
    write_entry({"program_partition", "in_interface%"}, inInterface_portion);
}

void PartitionStatistics::reportOutInterface()
{
    std::vector<std::string> outInterface;
    outInterface.reserve(m_partition.getOutInterface().size());
    std::transform(m_partition.getOutInterface().begin(), m_partition.getOutInterface().end(), std::back_inserter(outInterface),
            [] (llvm::Function* F) { return F->getName().str();});
    write_entry({"program_partition", "out_interface"}, outInterface);
    write_entry({"program_partition", "out_interface_size"}, (unsigned) outInterface.size());
    double outInterface_portion = (m_partition.getOutInterface().size() * 100.0) / m_module.size();
    write_entry({"program_partition", "out_interface%"}, outInterface_portion);
}

void PartitionStatistics::reportGlobals()
{
    std::vector<std::string> globals;
    globals.reserve(m_partition.getGlobals().size());
    std::transform(m_partition.getGlobals().begin(), m_partition.getGlobals().end(), std::back_inserter(globals),
            [] (llvm::GlobalVariable* global) { return global->getName().str();});
    write_entry({"program_partition", "globals"}, globals);
    write_entry({"program_partition", "globals_size"}, (unsigned) globals.size());
    double globals_portion = (m_partition.getGlobals().size() * 100.0) / m_module.getGlobalList().size();
    write_entry({"program_partition", "globals%"}, globals_portion);
}

void PartitionStatistics::reportNumOfContextSwitches()
{
    int ctxSwitchN = 0;
    for (const auto& F : m_partition.getPartition()) {
        ctxSwitchN += getCtxSwitchesInFunction(F);
    }
    write_entry({"program_partition", "context_switches"}, ctxSwitchN);
}

void PartitionStatistics::reportSizeOfTCB()
{
    int tcbSize = 0;
    for (const auto& F : m_partition.getPartition()) {
        if (!m_callgraph.hasFunctionNode(F)) {
            continue;
        }
        auto Fnode = m_callgraph.getFunctionNode(F);
        if (!Fnode->getWeight().hasFactor(WeightFactor::SIZE)) {
            tcbSize += Utils::getFunctionSize(F);
        } else {
            tcbSize += (int) Fnode->getWeight().getFactor(WeightFactor::SIZE).getValue();
        }
    }
    write_entry({"program_partition", "TCB"}, tcbSize);
}

void PartitionStatistics::repotArgsPassedAccrossPartition()
{
    int argNum = 0;
    for (const auto& F : m_partition.getPartition()) {
        argNum += getArgNumPassedFromFunction(F);
    }
    write_entry({"program_partition", "args_passed"}, argNum);
}

int PartitionStatistics::getCtxSwitchesInFunction(llvm::Function* F)
{
    int ctxSwitchN = 0;
    if (!m_callgraph.hasFunctionNode(F)) {
        return ctxSwitchN;
    }
    auto Fnode = m_callgraph.getFunctionNode(F);
    for (auto it = Fnode->inEdgesBegin(); it != Fnode->inEdgesEnd(); ++it) {
        auto* sourceF = it->getSource()->getFunction();
        if (!m_partition.contains(sourceF)) {
            const auto& callNum = it->getWeight().getFactor(WeightFactor::CALL_NUM).getValue();
            ctxSwitchN += (int) callNum;
        }
    }
    for (auto it = Fnode->outEdgesBegin(); it != Fnode->outEdgesEnd(); ++it) {
        auto* sinkF = it->getSink()->getFunction();
        if (!m_partition.contains(sinkF)) {
            const auto& callNum = it->getWeight().getFactor(WeightFactor::CALL_NUM).getValue();
            ctxSwitchN += (int) callNum;
        }
    }
    return ctxSwitchN;
}

int PartitionStatistics::getArgNumPassedFromFunction(llvm::Function* F)
{
    int argNum = 0;
    if (!m_callgraph.hasFunctionNode(F)) {
        return argNum;
    }
    auto Fnode = m_callgraph.getFunctionNode(F);
    for (auto it = Fnode->inEdgesBegin(); it != Fnode->inEdgesEnd(); ++it) {
        auto* sourceF = it->getSource()->getFunction();
        if (m_partition.contains(sourceF)) {
            continue;
        }
        int callNum = (int) it->getWeight().getFactor(WeightFactor::CALL_NUM).getValue();
        int argsPassed = (int) it->getWeight().getFactor(WeightFactor::ARG_NUM).getValue();
        argNum += callNum * argsPassed;
    }
    for (auto it = Fnode->outEdgesBegin(); it != Fnode->outEdgesEnd(); ++it) {
        auto* sinkF = it->getSink()->getFunction();
        if (m_partition.contains(sinkF)) {
            continue;
        }
        int callNum = (int) it->getWeight().getFactor(WeightFactor::CALL_NUM).getValue();
        int argsPassed = (int) it->getWeight().getFactor(WeightFactor::ARG_NUM).getValue();
        argNum += callNum * argsPassed;
    }
    return argNum;
}

} // namespace vazgen
 
