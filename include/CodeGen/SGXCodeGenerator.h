#pragma once

#include "CodeGen/Function.h"
#include <unordered_map>

namespace vazgen {

class SGXCodeGenerator
{
public:
    using Functions = std::vector<Function>;

public:
    SGXCodeGenerator(const std::string& programName,
                     const Functions& secureFunctions,
                     const Functions& appFunctions)
	    : m_prefix(programName)
	    , m_enclaveFunctions(secureFunctions)
	    , m_appFunctions(appFunctions)
    {
    }

    SGXCodeGenerator(const SGXCodeGenerator&) = delete;
    SGXCodeGenerator(SGXCodeGenerator&&) = delete;
    SGXCodeGenerator& operator =(const SGXCodeGenerator&) = delete;
    SGXCodeGenerator& operator =(SGXCodeGenerator&&) = delete;

public:
    virtual void generate() = 0;

protected:
    const std::string& m_prefix;
    const Functions& m_enclaveFunctions;
    const Functions& m_appFunctions;
};

}

