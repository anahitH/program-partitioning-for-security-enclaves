#include "CodeGen/OpenEnclaveSupportedLibFunctions.h"

namespace vazgen {

namespace {

void fill(std::unordered_set<std::string>& functions)
{
    functions.insert("printf");
    functions.insert("fprintf");
    functions.insert("time");
    functions.insert("rand");
    functions.insert("srand");
    // TODO: fill
    // TODO: thread functions?
}

}

OpenEnclaveSupportedLibFunctions::OpenEnclaveSupportedLibFunctions()
{
    fill(m_supportedFunctions);
}

}

