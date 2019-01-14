#pragma once

#include "Utils/Annotation.h"
#include "Partition.h"

#include <memory>
#include <vector>
#include <unordered_set>

namespace llvm {
class Module;
}

namespace pdg {
class PDG;
}

namespace vazgen {

class Logger;

/// For internal uses only
class BlockPartitioner
{
public:
    using Annotations = std::vector<Annotation>;
    // can we have unique?
    using PDGType = std::shared_ptr<pdg::PDG>;

public:
    BlockPartitioner(llvm::Module& M, PDGType pdg, Logger& logger)
        : m_module(M)
        , m_pdg(pdg)
        , m_logger(logger)
    {
    }

public:
    BasicBlockPartition getSecurePartition() const
    {
        return m_securePartition;
    }

    BasicBlockPartition getInsecurePartition() const
    {
        return m_insecurePartition;
    }

public:
    void partition(const Annotations& annotations);

private:
    void computeInsecurePartition();

protected:
    llvm::Module& m_module;
    PDGType m_pdg;
    Logger& m_logger;
    BasicBlockPartition m_securePartition;
    BasicBlockPartition m_insecurePartition;
}; // class BlockPartitioner

} // namespace vazgen

