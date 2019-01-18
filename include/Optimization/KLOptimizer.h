#pragma once

#include <memory>

#include "Optimization/PartitionOptimization.h"

namespace pdg {
class PDG;
}

namespace vazgen {

class CallGraph;
class Partition;
class Logger;

class KLOptimizer : public PartitionOptimization
{
public:
    KLOptimizer(const CallGraph& callgraph,
                PDGType pdg,
                Partition& securePartition,
                Partition& insecurePartition,
                Logger& logger);

    KLOptimizer(const KLOptimizer& ) = delete;
    KLOptimizer(KLOptimizer&& ) = delete;
    KLOptimizer& operator =(const KLOptimizer& ) = delete;
    KLOptimizer& operator =(KLOptimizer&& ) = delete;

public:
    void run() override;

    static bool classof(const PartitionOptimization* opt)
    {
        return opt->getOptimizationType() == PartitionOptimizer::KERNIGHAN_LIN;
    }

private:
    class Impl;
    std::shared_ptr<Impl> m_impl;
}; // class KLOptimizer

} // namespace vazgen

