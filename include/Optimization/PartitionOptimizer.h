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

// TODO: think about different strategies for optimization, e.g. smaller TCB, fewer function calls across partitions, etc
class PartitionOptimizer
{
public:
    // Apply optimization in the given order
    enum Optimization {
        FUNCTIONS_MOVE_TO = 0,
        FUNCTIONS_MOVE_OUT,
        GLOBALS_MOVE_TO,
        GLOBALS_MOVE_OUT,
        DUPLICATE_FUNCTIONS,
        OPT_NUM
    };

    using OptimizationTy = std::shared_ptr<PartitionOptimization>;
    using PDGType = std::shared_ptr<pdg::PDG>;
    using LoopInfoGetter = std::function<llvm::LoopInfo* (llvm::Function*)>;

public:
    explicit PartitionOptimizer(Partition& partition);

    PartitionOptimizer(const PartitionOptimizer& ) = delete;
    PartitionOptimizer(PartitionOptimizer&& ) = delete;
    PartitionOptimizer& operator= (const PartitionOptimizer& ) = delete;
    PartitionOptimizer& operator= (PartitionOptimizer&& ) = delete;

public:
    void setPDG(PDGType pdg);
    void setLoopInfoGetter(const LoopInfoGetter& loopInfoGetter);

public:
    virtual void run();

private:
    OptimizationTy getOptimizerFor(Optimization opt);
    void collectAvailableOptimizations();
    void apply();

protected:
    Partition& m_partition;
    PDGType m_pdg;
    LoopInfoGetter m_loopInfoGetter;
    std::vector<OptimizationTy> m_optimizations;
}; // class PartitionOptimizer

} // namespace vazgen

