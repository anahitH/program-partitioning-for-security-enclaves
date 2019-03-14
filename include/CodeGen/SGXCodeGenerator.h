#pragma once

#include "CodeGen/Function.h"
#include "CodeGen/SourceFile.h"

#include <vector>
#include <unordered_map>

namespace vazgen {

class SGXCodeGenerator
{
public:
    using Functions = std::vector<Function>;

public:
    SGXCodeGenerator(const std::string& programName,
                     const Functions& secureFunctions,
                     const Functions& appFunctions);

    SGXCodeGenerator(const SGXCodeGenerator&) = delete;
    SGXCodeGenerator(SGXCodeGenerator&&) = delete;
    SGXCodeGenerator& operator =(const SGXCodeGenerator&) = delete;
    SGXCodeGenerator& operator =(SGXCodeGenerator&&) = delete;

    void generate();

private:
    void generateInterfaceSelectors();

private:
    const std::string& m_prefix;
    const Functions& m_enclaveFunctions;
    const Functions& m_appFunctions;

    std::unordered_map<std::string, std::string> m_ecallSelectorsMap;
    std::unordered_map<std::string, std::string> m_ocallSelectorsMap;
    SourceFile m_interfaceSelectorsFile;
}; // class SGXCodeGenerator

} // namespace vazgen

