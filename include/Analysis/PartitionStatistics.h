#pragma once

#include "Utils/Statistics.h"

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
                        const Partition& partition,
                        const CallGraph& callgraph,
                        llvm::Module& M);

    void report() final;

private:
    void reportPartitionFunctions();
    void reportSecurityRelatedFunctions();
    void reportInInterface();
    void reportOutInterface();
    void reportGlobals();
    void reportNumOfContextSwitches();
    void reportSizeOfTCB();
    void repotArgsPassedAccrossPartition();

    int getCtxSwitchesInFunction(llvm::Function* F);
    int getArgNumPassedFromFunction(llvm::Function* F);

private:
    const Partition& m_partition;
    const CallGraph& m_callgraph;
    llvm::Module& m_module;
}; // class PartitionStatistics


} // namespace vazgen

