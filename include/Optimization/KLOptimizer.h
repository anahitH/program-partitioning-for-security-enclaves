#pragma once

#include <memory>

namespace pdg {
class PDG;
}

namespace vazgen {

class CallGraph;
class Partition;
class Logger;

class KLOptimizer
{
public:
    KLOptimizer(const CallGraph& callgraph,
                const pdg::PDG& pdg,
                Partition& securePartition,
                Partition& insecurePartition,
                Logger& logger);

    KLOptimizer(const KLOptimizer& ) = delete;
    KLOptimizer(KLOptimizer&& ) = delete;
    KLOptimizer& operator =(const KLOptimizer& ) = delete;
    KLOptimizer& operator =(KLOptimizer&& ) = delete;

public:
    void run();

private:
    class Impl;
    std::shared_ptr<Impl> m_impl;
}; // class KLOptimizer

} // namespace vazgen

