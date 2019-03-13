#pragma once

#include <string>

namespace clang {
class Type;
}

namespace vazgen {

class ClangToolUtils
{
public:
    static std::string getScalarTypeName(const clang::Type* type);
    static std::string getTypeName(const clang::Type* type);
}; // class ClangToolUtils

} // namespace vazgen

