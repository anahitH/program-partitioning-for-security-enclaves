#pragma once

#include "CodeGen/SGXSupportedLibFunctions.h"

#include <unordered_set>

namespace vazgen {

class OpenEnclaveSupportedLibFunctions final : public SGXSupportedLibFunctions
{
public:
    static OpenEnclaveSupportedLibFunctions& get()
    {
        static OpenEnclaveSupportedLibFunctions instance;
        return instance;
    }

    OpenEnclaveSupportedLibFunctions(const OpenEnclaveSupportedLibFunctions& ) = delete;
    OpenEnclaveSupportedLibFunctions(OpenEnclaveSupportedLibFunctions&& ) = delete;
    OpenEnclaveSupportedLibFunctions& operator =(const OpenEnclaveSupportedLibFunctions& ) = delete;
    OpenEnclaveSupportedLibFunctions& operator =(OpenEnclaveSupportedLibFunctions&& ) = delete;

private:
    OpenEnclaveSupportedLibFunctions();

public:
    bool supportsFunction(const std::string& f_name) const override
    {
        return m_supportedFunctions.find(f_name) != m_supportedFunctions.end();
    }

private:
    std::unordered_set<std::string> m_supportedFunctions;
}; // class OpenEnclaveSupportedLibFunctions

}

