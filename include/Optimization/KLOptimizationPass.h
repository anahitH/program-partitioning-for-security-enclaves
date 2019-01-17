#pragma once

#include <memory>
#include <vector>

namespace llvm {
class Function;
}

namespace vazgen {

class CallGraph;
class Partition;
class Logger;

class KLOptimizationPass
{
public:
    using Functions = std::vector<llvm::Function*>;

public:
    KLOptimizationPass(const CallGraph& callgraph,
                       Partition& securePartition,
                       Partition& insecurePartition,
                       Logger& logger);

    KLOptimizationPass(const KLOptimizationPass& ) = delete;
    KLOptimizationPass(KLOptimizationPass&& ) = delete;
    KLOptimizationPass& operator =(const KLOptimizationPass& ) = delete;
    KLOptimizationPass& operator =(KLOptimizationPass&& ) = delete;

public:
    void setCandidates(const Functions& candidates);

    void run();

private:
    class Impl;
    std::shared_ptr<Impl> m_impl;
};

} // namespace vazgen

