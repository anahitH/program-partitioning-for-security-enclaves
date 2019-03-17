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
    if (m_isArray) {
        typeStr << "[]";
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
    if (m_isExtern) {
        declStr << "extern ";
    }
    if (!m_declPrefix.empty()) {
        declStr << m_declPrefix << " ";
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
    return declStr.str();
}

std::string Function::getDefinitionAsString() const
{
    if (m_isExtern && m_body.empty()) {
        return getDeclarationAsString();
    }
    std::stringstream defStr;
    std::string decl = getDeclarationAsString();
    decl = decl.substr(0, decl.size() - 1);
    defStr << decl << " { \n";
    for (const auto& instr : m_body) {
        defStr << "   " << instr << ";\n";
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
    callStr << ")";
    return callStr.str();
}

} // namespace vazgen

