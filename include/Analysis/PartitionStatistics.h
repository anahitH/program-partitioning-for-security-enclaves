#pragma once

#include "Utils/Statistics.h"
#include "Analysis/Numbers.h"

#include <fstream>

namespace llvm {
class Module;
class Function;
}

namespace vazgen {

class Partition;
class CallGraph;

class PartitionStatistics : public Statistics
{
public:
    PartitionStatistics(std::ofstream& strm,
                        const Partition& securePartition,
                        const Partition& insecurePartition,
                        const CallGraph& callgraph,
                        llvm::Module& M);

    void report() final;

private:
    void report(const Partition& partition);
    void reportPartitionFunctions(const Partition& partition);
    void reportSecurityRelatedFunctions(const Partition& partition);
    void reportInInterface(const Partition& partition);
    void reportOutInterface(const Partition& partition);
    void reportGlobals(const Partition& partition);
    void reportNumOfContextSwitches(const Partition& partition);
    void reportSizeOfTCB(const Partition& partition);
    void repotArgsPassedAccrossPartition(const Partition& partition);

    Double getCtxSwitchesInFunction(llvm::Function* F, const Partition& partition);
    Double getArgNumPassedFromFunction(llvm::Function* F, const Partition& partition);

private:
    std::string m_partitionName;
    const Partition& m_securePartition;
    const Partition& m_insecurePartition;
    const CallGraph& m_callgraph;
    llvm::Module& m_module;
    unsigned m_moduleSize;
}; // class PartitionStatistics


} // namespace vazgen

