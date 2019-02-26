#include "CodeGen/Function.h"

#include <sstream>

namespace vazgen {

std::string Type::getAsString() const
{
    std::stringstream typeStr;
    typeStr << m_qualifier;
    if (!m_qualifier.empty()) {
        typeStr << " ";
    }
    typeStr << m_name;
    if (m_isPtr) {
        typeStr << "*";
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
    if (m_isStatic) {
        declStr << "static ";
    }
    declStr << m_returnType.getAsString() << " ";
    declStr << m_name
            << "(";
    int i = 0;
    for (const auto& param : m_params) {
        declStr << param.getAsString();
        if (i++ != m_params.size() - 1) {
            declStr << ", ";
        }
    }
    declStr << ");";
    declStr.seekp(-3, std::ios_base::end);
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
           << " " << className << "::";
    defStr << m_name
            << "(";
    int i = 0;
    for (const auto& param : m_params) {
        defStr << param.getAsString();
        if (i++ != m_params.size() - 1) {
            defStr << ", ";
        }
    }
    defStr << ") \n{\n";
    for (const auto& instr : m_body) {
        defStr << "   " << instr << "\n";
    }
    defStr << "}";
    return defStr.str();
}

std::string Function::getCallAsString(const std::vector<std::string>& arguments) const
{
    std::stringstream callStr;
    callStr << m_name << "(";
    int i = 0;
    for (const std::string& arg : arguments) {
        if (m_params[i++].m_type.m_isPtr) {
            callStr << "&";
        }
        callStr << arg;
        if (arg != arguments.back()) {
            callStr << ", ";
        }
    }
    callStr << ");";
    return callStr.str();
}

} // namespace vazgen

