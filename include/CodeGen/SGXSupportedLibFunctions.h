#pragma once

#include <string>

namespace vazgen {

class SGXSupportedLibFunctions
{
public:
    virtual bool supportsFunction(const std::string& f_name) const = 0;
}; // class SGXSupportedLibFunctions

}

