#pragma once

#include "Annotation.h"
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

/// For internal uses only
class Partitioner
{
public:
    using Annotations = std::vector<Annotation>;
    // can we have unique?
    using PDGType = std::shared_ptr<pdg::PDG>;

public:
    Partitioner(llvm::Module& M, PDGType pdg)
        : m_module(M)
        , m_pdg(pdg)
    {
    }

public:
    Partition partition(const Annotations& annotations);

protected:
    llvm::Module& m_module;
    PDGType m_pdg;
}; // class Partitioner

} // namespace vazgen

