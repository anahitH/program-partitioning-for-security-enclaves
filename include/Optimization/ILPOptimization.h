#pragma once

#include "Optimization/PartitionOptimization.h"
#include <memory>

namespace vazgen {

class CallGraph;
class Logger;

class ILPOptimization : public PartitionOptimization
{
public:
    ILPOptimization(const CallGraph& callgraph,
                    Partition& securePartition,
                    Partition& insecurePartition,
                    Logger& logger);

    ILPOptimization(const ILPOptimization& ) = delete;
    ILPOptimization(ILPOptimization&& ) = delete;
    ILPOptimization& operator =(const ILPOptimization& ) = delete;
    ILPOptimization& operator =(ILPOptimization&& ) = delete;

public:
    void run() override;
    void apply() override;

    static bool classof(const PartitionOptimization* opt)
    {
        return opt->getOptimizationType() == PartitionOptimizer::ILP;
    }

private:
    class Impl;
    std::shared_ptr<Impl> m_impl;
}; // class ILPOptimization

} // namespace vazgen

