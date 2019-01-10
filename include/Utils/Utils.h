#pragma once

#include <string>

namespace llvm {
class Module;
class Function;
}

namespace pdg {
class PDGNode;
}

namespace vazgen {

class Utils
{
public:
    static void saveModule(llvm::Module* M, const std::string& name);

    // TODO: consider keeping node parent information in a node itself.
    static llvm::Function* getNodeParent(pdg::PDGNode* node);

    static int getFunctionSize(llvm::Function* F);
}; // class Utils

} // namespace vazgen

