#pragma once

#include <string>
#include <unordered_set>

namespace vazgen {

class Logger;

class CodeGenUtils
{
public:
    using Functions =  std::unordered_set<std::string>;

    static Functions parseFunctions(const std::string& fileName);
    static std::pair<Functions, Functions> parseFunctionsFromStats(const std::string& statsFile, Logger& logger);
}; // class CodeGenUtils

} // namespace vazgen

