#pragma once

#include <string>

namespace llvm {
class Module;
}

namespace vazgen {

class Utils
{
public:
    static void saveModule(llvm::Module* M, const std::string& name);
}; // class Utils

} // namespace vazgen

