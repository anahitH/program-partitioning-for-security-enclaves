#include "Analysis/PartitionStatistics.h"

#include "Analysis/CallGraph.h"
#include "Analysis/Partition.h"
#include "Utils/Utils.h"

#include "llvm/IR/Module.h"
#include "llvm/IR/Function.h"

#include "llvm/Support/Debug.h"
#include "llvm/Support/raw_ostream.h"

namespace vazgen {

PartitionStatistics::PartitionStatistics(std::ofstream& strm,
                                         const Partition& securePartition,
                                         const Partition& insecurePartition,
                                         const CallGraph& callgraph,
                                         llvm::Module& M)
    : Statistics(strm, Statistics::JSON)
    , m_securePartition(securePartition)
    , m_insecurePartition(insecurePartition)
    , m_callgraph(callgraph)
    , m_module(M)
    , m_moduleSize(0)
{
    for (auto& F : m_module) {
        if (!F.isDeclaration()) {
            m_moduleSize += Utils::getFunctionSize(&F);
        }
    }
}

void PartitionStatistics::report()
{
    m_partitionName = "secure_partition";
    report(m_securePartition);
    m_partitionName = "insecure_partition";
    report(m_insecurePartition);
    flush();
}

void PartitionStatistics::report(const Partition& partition)
{
    reportPartitionFunctions(partition);
    reportSecurityRelatedFunctions(partition);
    reportInInterface(partition);
    //reportOutInterface();
    //reportGlobals();
    reportNumOfContextSwitches(partition);
    reportSizeOfTCB(partition);
    repotArgsPassedAccrossPartition(partition);
}

void PartitionStatistics::reportPartitionFunctions(const Partition& partition)
{
    std::vector<std::string> partitionFs;
    partitionFs.reserve(partition.getPartition().size());
    std::transform(partition.getPartition().begin(), partition.getPartition().end(), std::back_inserter(partitionFs),
            [] (llvm::Function* F) { if (F) { return F->getName().str(); } });
    write_entry({"partition", m_partitionName, "partition_functions"}, partitionFs);
    write_entry({"partition", m_partitionName, "partition_size"}, (unsigned) partitionFs.size());
    double partition_portion = (partition.getPartition().size() * 100.0) / m_module.size();
    write_entry({"partition", m_partitionName, "partition%"}, partition_portion);
}

void PartitionStatistics::reportSecurityRelatedFunctions(const Partition& partition)
{
    std::vector<std::string> staticAnalysisFs;
    staticAnalysisFs.reserve(partition.getRelatedFunctions().size());
    std::transform(partition.getRelatedFunctions().begin(), partition.getRelatedFunctions().end(), std::back_inserter(staticAnalysisFs),
            [] (const auto& pair) { return pair.first->getName().str() + "  " + std::to_string(pair.second);});
    write_entry({"partition", m_partitionName, "security_related_functions"}, staticAnalysisFs);
    write_entry({"partition", m_partitionName, "security_related_functions_size"}, (unsigned) staticAnalysisFs.size());
    double security_related_portion = (partition.getRelatedFunctions().size() * 100.0) / m_module.size();
    write_entry({"partition", m_partitionName, "security_related%"}, security_related_portion);
}

void PartitionStatistics::reportInInterface(const Partition& partition)
{
    std::vector<std::string> inInterface;
    inInterface.reserve(partition.getInInterface().size());
    std::transform(partition.getInInterface().begin(), partition.getInInterface().end(), std::back_inserter(inInterface),
            [] (llvm::Function* F) { return F->getName().str();});
    write_entry({"partition", m_partitionName, "in_interface"}, inInterface);
    write_entry({"partition", m_partitionName, "in_interface_size"}, (unsigned) inInterface.size());
    double inInterface_portion = (partition.getInInterface().size() * 100.0) / m_module.size();
    write_entry({"partition", "m_partitionName", "in_interface%"}, inInterface_portion);
}

void PartitionStatistics::reportOutInterface(const Partition& partition)
{
    std::vector<std::string> outInterface;
    outInterface.reserve(partition.getOutInterface().size());
    std::transform(partition.getOutInterface().begin(), partition.getOutInterface().end(), std::back_inserter(outInterface),
            [] (llvm::Function* F) { return F->getName().str();});
    write_entry({"partition", m_partitionName, "out_interface"}, outInterface);
    write_entry({"partition", m_partitionName, "out_interface_size"}, (unsigned) outInterface.size());
    double outInterface_portion = (partition.getOutInterface().size() * 100.0) / m_module.size();
    write_entry({"partition", m_partitionName, "out_interface%"}, outInterface_portion);
}

void PartitionStatistics::reportGlobals(const Partition& partition)
{
    std::vector<std::string> globals;
    globals.reserve(partition.getGlobals().size());
    std::transform(partition.getGlobals().begin(), partition.getGlobals().end(), std::back_inserter(globals),
            [] (llvm::GlobalVariable* global) { return global->getName().str();});
    write_entry({"partition", m_partitionName, "globals"}, globals);
    write_entry({"partition", m_partitionName, "globals_size"}, (unsigned) globals.size());
    double globals_portion = (partition.getGlobals().size() * 100.0) / m_module.getGlobalList().size();
    write_entry({"partition", m_partitionName, "globals%"}, globals_portion);
}

void PartitionStatistics::reportNumOfContextSwitches(const Partition& partition)
{
    Double ctxSwitchN;
    for (const auto& F : partition.getPartition()) {
        ctxSwitchN += getCtxSwitchesInFunction(F, partition);
    }
    write_entry({"partition", m_partitionName, "context_switches"}, (double) ctxSwitchN);
}

void PartitionStatistics::reportSizeOfTCB(const Partition& partition)
{
    long tcbSize = 0;
    for (const auto& F : partition.getPartition()) {
        if (!m_callgraph.hasFunctionNode(F)) {
            continue;
        }
        tcbSize += Utils::getFunctionSize(F);
    }
    double tcb_portion = (tcbSize * 100.0) / m_moduleSize;
    write_entry({"partition", m_partitionName, "TCB"}, (double) tcbSize);
    write_entry({"partition", m_partitionName, "TCB%"}, (double) tcb_portion);
}

void PartitionStatistics::repotArgsPassedAccrossPartition(const Partition& partition)
{
    Double argNum = 0;
    for (const auto& F : partition.getPartition()) {
        argNum += getArgNumPassedFromFunction(F, partition);
    }
    write_entry({"partition", m_partitionName, "args_passed"}, (double) argNum);
}

Double PartitionStatistics::getCtxSwitchesInFunction(llvm::Function* F, const Partition& partition)
{
    Double ctxSwitchN = 0;
    if (!m_callgraph.hasFunctionNode(F)) {
        return ctxSwitchN;
    }
    auto Fnode = m_callgraph.getFunctionNode(F);
    for (auto it = Fnode->inEdgesBegin(); it != Fnode->inEdgesEnd(); ++it) {
        auto* sourceF = it->getSource()->getFunction();
        if (!partition.contains(sourceF)) {
            const auto& callNum = it->getWeight().getFactor(WeightFactor::CALL_NUM).getValue();
            ctxSwitchN += callNum;
        }
    }
    for (auto it = Fnode->outEdgesBegin(); it != Fnode->outEdgesEnd(); ++it) {
        auto* sinkF = it->getSink()->getFunction();
        if (!partition.contains(sinkF)) {
            const auto& callNum = it->getWeight().getFactor(WeightFactor::CALL_NUM).getValue();
            ctxSwitchN += callNum;
        }
    }
    return ctxSwitchN;
}

Double PartitionStatistics::getArgNumPassedFromFunction(llvm::Function* F, const Partition& partition)
{
    Double argNum = 0;
    if (!m_callgraph.hasFunctionNode(F)) {
        return argNum;
    }
    auto Fnode = m_callgraph.getFunctionNode(F);
    for (auto it = Fnode->inEdgesBegin(); it != Fnode->inEdgesEnd(); ++it) {
        auto* sourceF = it->getSource()->getFunction();
        if (partition.contains(sourceF)) {
            continue;
        }
        const auto& callNum = it->getWeight().getFactor(WeightFactor::CALL_NUM).getValue();
        const auto& argsPassed = it->getWeight().getFactor(WeightFactor::ARG_NUM).getValue();
        argNum += callNum * argsPassed;
    }
    for (auto it = Fnode->outEdgesBegin(); it != Fnode->outEdgesEnd(); ++it) {
        auto* sinkF = it->getSink()->getFunction();
        if (partition.contains(sinkF)) {
            continue;
        }
        const auto& callNum = it->getWeight().getFactor(WeightFactor::CALL_NUM).getValue();
        const auto& argsPassed = it->getWeight().getFactor(WeightFactor::ARG_NUM).getValue();
        argNum += callNum * argsPassed;
    }
    return argNum;
}

} // namespace vazgen
 
