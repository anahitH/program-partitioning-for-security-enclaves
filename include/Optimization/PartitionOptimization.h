#pragma once

#include "Analysis/Partition.h"

#include <memory>
#include <vector>

namespace pdg {
class PDG;
}

namespace vazgen {

class PartitionOptimization
{
public:
    using PDGType = std::shared_ptr<pdg::PDG>;

public:
    PartitionOptimization(Partition& partition, PDGType pdg);

    PartitionOptimization(const PartitionOptimization& ) = delete;
    PartitionOptimization(PartitionOptimization&& ) = delete;
    PartitionOptimization& operator= (const PartitionOptimization& ) = delete;
    PartitionOptimization& operator= (PartitionOptimization&& ) = delete;

    virtual ~PartitionOptimization() = default;

public:
    virtual void run() = 0;
    
protected:
    Partition& m_partition;
    PDGType m_pdg;
}; // class PartitionOptimization

} // namespace vazgen

