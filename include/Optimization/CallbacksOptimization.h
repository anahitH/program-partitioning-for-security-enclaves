#pragma once

#include "Optimization/PartitionOptimization.h"

namespace pdg {
class PDG;
}

namespace vazgen {

class CallGraph;
class Logger;

class CallbacksOptimization : public PartitionOptimization
{
public:
    CallbacksOptimization(const CallGraph& callgraph,
                          Partition& securePartition,
                          Partition& insecurePartition,
                          Logger& logger);

    CallbacksOptimization(const CallbacksOptimization& ) = delete;
    CallbacksOptimization(CallbacksOptimization&& ) = delete;
    CallbacksOptimization& operator =(const CallbacksOptimization& ) = delete;
    CallbacksOptimization& operator =(CallbacksOptimization&& ) = delete;

public:
    void run() override;

    static bool classof(const PartitionOptimization* opt)
    {
        return opt->getOptimizationType() == PartitionOptimizer::ADJUST_CALLBACKS;
    }

private:
    void moveFunctionsToSecurePartition(const std::vector<llvm::Function*>& functions);

private:
    Partition& m_securePartition;
    Partition& m_insecurePartition;
    const CallGraph& m_callGraph;
}; // class CallbacksOptimization

} // namespace vazgen

