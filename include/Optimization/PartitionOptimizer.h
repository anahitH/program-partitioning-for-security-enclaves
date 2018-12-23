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

// TODO: think about different strategies for optimization, e.g. smaller TCB, fewer function calls across partitions, etc
class PartitionOptimizer
{
public:
    // Apply optimization in the given order
    enum Optimization {
        FUNCTIONS_MOVE_TO = 0,
        GLOBALS_MOVE_TO,
        DUPLICATE_FUNCTIONS,
        OPT_NUM
    };

    using OptimizationTy = std::shared_ptr<PartitionOptimization>;
    using PDGType = std::shared_ptr<pdg::PDG>;
    using LoopInfoGetter = std::function<llvm::LoopInfo* (llvm::Function*)>;

public:
    PartitionOptimizer(Partition& securePartition,
                       Partition& insecurePartition,
                       PDGType pdg,
                       Logger& logger);

    PartitionOptimizer(const PartitionOptimizer& ) = delete;
    PartitionOptimizer(PartitionOptimizer&& ) = delete;
    PartitionOptimizer& operator= (const PartitionOptimizer& ) = delete;
    PartitionOptimizer& operator= (PartitionOptimizer&& ) = delete;

public:
    void setLoopInfoGetter(const LoopInfoGetter& loopInfoGetter);

public:
    virtual void run();

private:
    OptimizationTy getOptimizerFor(Optimization opt,
                                   Partition& partition,
                                   const Partition& complementPart);
    void collectAvailableOptimizations();
    void runDuplicateFunctionsOptimization(OptimizationTy opt);
    void apply();

protected:
    Partition& m_securePartition;
    Partition& m_insecurePartition;
    PDGType m_pdg;
    Logger& m_logger;
    LoopInfoGetter m_loopInfoGetter;
    std::vector<OptimizationTy> m_optimizations;
}; // class PartitionOptimizer

} // namespace vazgen

