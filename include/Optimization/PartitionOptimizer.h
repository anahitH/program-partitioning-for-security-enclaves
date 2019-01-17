#pragma once

#include "Analysis/Partition.h"

#include <memory>
#include <vector>
#include <functional>

namespace pdg {
class PDG;
}

namespace llvm {
class LoopInfo;
}

namespace vazgen {

class PartitionOptimization;
class Logger;
class CallGraph;

// TODO: think about different strategies for optimization, e.g. smaller TCB, fewer function calls across partitions, etc
class PartitionOptimizer
{
public:
    // Apply optimization in the given order
    enum Optimization {
        FUNCTIONS_MOVE_TO = 0,
        GLOBALS_MOVE_TO,
        DUPLICATE_FUNCTIONS,
        KERNIGHAN_LIN,
        OPT_NUM
    };

    using OptimizationTy = std::shared_ptr<PartitionOptimization>;
    using PDGType = std::shared_ptr<pdg::PDG>;
    using LoopInfoGetter = std::function<llvm::LoopInfo* (llvm::Function*)>;
    using Optimizations = std::vector<Optimization>;

public:
    PartitionOptimizer(Partition& securePartition,
                       Partition& insecurePartition,
                       PDGType pdg,
                       const CallGraph& callGraph,
                       Logger& logger);

    PartitionOptimizer(const PartitionOptimizer& ) = delete;
    PartitionOptimizer(PartitionOptimizer&& ) = delete;
    PartitionOptimizer& operator= (const PartitionOptimizer& ) = delete;
    PartitionOptimizer& operator= (PartitionOptimizer&& ) = delete;

public:
    void setLoopInfoGetter(const LoopInfoGetter& loopInfoGetter);

public:
    virtual void run(const Optimizations& opts);

private:
    OptimizationTy getOptimizerFor(Optimization opt,
                                   Partition& partition,
                                   const Partition& complementPart);
    void runDuplicateFunctionsOptimization(OptimizationTy opt);
    void apply();

protected:
    Partition& m_securePartition;
    Partition& m_insecurePartition;
    PDGType m_pdg;
    const CallGraph& m_callgraph;
    Logger& m_logger;
    LoopInfoGetter m_loopInfoGetter;
    std::vector<OptimizationTy> m_optimizations;
}; // class PartitionOptimizer

} // namespace vazgen

