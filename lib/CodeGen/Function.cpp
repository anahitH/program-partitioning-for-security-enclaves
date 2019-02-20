#include "CodeGen/Function.h"

#include <sstream>

namespace vazgen {

std::string Type::getAsString() const
{
    std::stringstream typeStr;
    typeStr << m_qualifier
            << m_name;
    if (m_isPtr) {
        typeStr << "* ";
    }
    return typeStr.str();
}

std::string Variable::getAsString() const
{
    return m_type.getAsString() + " " + m_name;
}

std::string Function::getDeclarationAsString() const
{
    std::stringstream declStr;
    declStr << m_returnType.getAsString();
    declStr << m_name
            << "(";
    for (const auto& param : m_params) {
        declStr << param.getAsString() << ",";
    }
    declStr.seekp(-1, std::ios_base::end);
    declStr << ");";
    return declStr.str();
}

std::string Function::getDefinitionAsString() const
{
    std::stringstream defStr;
    defStr << getDeclarationAsString();
    defStr.seekp(-1, std::ios_base::end);
    defStr << " {";
    for (const auto& instr : m_body) {
        defStr << instr << ";\n";
    }
    defStr << "}";
    return defStr.str();
}

std::string Function::getDefinitionAsString(const std::string& className) const
{
    std::stringstream defStr;
    defStr << m_returnType.getAsString()
           << className << "::";
    defStr << m_name
            << "(";
    for (const auto& param : m_params) {
        defStr << param.getAsString() << ",";
    }
    defStr.seekp(-1, std::ios_base::end);
    defStr << ") {";
    for (const auto& instr : m_body) {
        defStr << instr << ";\n";
    }
    defStr << "}";
    return defStr.str();
}

std::string Function::getCallAsString(const std::vector<std::string>& arguments) const
{
    std::stringstream callStr;
    callStr << m_name << "(";
    for (const std::string& arg : arguments) {
        callStr << arg;
        if (arg != arguments.back()) {
            callStr << ")";
        }
    }
    callStr << ");\n";
    return callStr.str();
}

} // namespace vazgen

