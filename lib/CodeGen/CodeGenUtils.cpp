#include "CodeGen/CodeGenUtils.h"
#include "Utils/Logger.h"

#include "nlohmann/json.hpp"

#include <fstream>

namespace vazgen {

CodeGenUtils::Functions CodeGenUtils::parseFunctions(const std::string& fileName)
{
    Functions functions;
    std::ifstream fileStream(fileName);
    std::string function;
    while (std::getline(fileStream, function)) {
        functions.insert(function);
    }
    return functions;
}

std::pair<CodeGenUtils::Functions, CodeGenUtils::Functions>
CodeGenUtils::parseFunctionsFromStats(const std::string& statsFile, Logger& logger)
{
    using namespace nlohmann;
    std::pair<Functions, Functions> result;
    std::ifstream ifs (statsFile, std::ifstream::in);
    if (!ifs.is_open()) {
        logger.error("Could not open annotations' json file " + statsFile + "\n");
        return result;
    }
    json stats;
    ifs >> stats;
    const auto& secureFunctions = stats["partition"]["secure_partition"]["in_interface"];
    result.first.insert(secureFunctions.begin(), secureFunctions.end());
    const auto& insecureFunctions = stats["partition"]["insecure_partition"]["in_interface"];
    result.second.insert(insecureFunctions.begin(), insecureFunctions.end());
}

} // namespace vazgen

