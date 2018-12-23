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
class Partitioner
{
public:
    using Annotations = std::vector<Annotation>;
    // can we have unique?
    using PDGType = std::shared_ptr<pdg::PDG>;

public:
    Partitioner(llvm::Module& M, PDGType pdg, Logger& logger)
        : m_module(M)
        , m_pdg(pdg)
        , m_logger(logger)
    {
    }

public:
    Partition getSecurePartition() const
    {
        return m_securePartition;
    }

    Partition getInsecurePartition() const
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
    Partition m_securePartition;
    Partition m_insecurePartition;
}; // class Partitioner

} // namespace vazgen

