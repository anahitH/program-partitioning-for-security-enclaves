#pragma once

#include "CodeGen/Class.h"
#include <vector>

namespace vazgen {

class SourceFile
{
public:
    using VariableValue = std::pair<Variable, std::string>;

public:
    SourceFile() = default;
    SourceFile(std::string name, bool isHeader)
        : m_name(name)
        , m_isHeader(isHeader)
    {
    }

public:
    const std::string& getName() const
    {
        return m_name;
    }

    bool isHeader() const
    {
        return m_isHeader;
    }

    const std::vector<std::string>& getIncludes() const
    {
        return m_includes;
    }

    const std::vector<std::string>& getMacros() const
    {
        return m_macros;
    }

    const std::vector<std::string>& getNamespaces() const
    {
        return m_nestedNamespaces;
    }

    const std::vector<Class>& getClasses() const
    {
        return m_classes;
    }

    const std::vector<Function>& getFunctions() const
    {
        return m_functions;
    }

    const std::vector<VariableValue>& getGlobalVariables() const
    {
        return m_globalVariables;
    }

    void setName(const std::string name)
    {
        m_name = name;
    }

    void setHeader(bool isHeader)
    {
        m_isHeader = isHeader;
    }

    void setIncludes(const std::vector<std::string>& includes)
    {
        m_includes = includes;
    }

    void setMacros(const std::vector<std::string>& macros)
    {
        m_macros = macros;
    }

    void setNamespaces(const std::vector<std::string>& namespaces)
    {
        m_nestedNamespaces = namespaces;
    }

    void addNamespace(const std::string& namespace_)
    {
        m_nestedNamespaces.push_back(namespace_);
    }

    void setClasses(const std::vector<Class>& classes)
    {
        m_classes = classes;
    }

    void setFunctions(const std::vector<Function>& functions)
    {
        m_functions = functions;
    }

    void setGlobalVariables(const std::vector<VariableValue>& globals)
    {
        m_globalVariables = globals;
    }

    void addInclude(const std::string& include)
    {
        m_includes.push_back(include);
    }

    void addMacro(const std::string& macro)
    {
        m_macros.push_back(macro);
    }

    void addClass(const Class& class_)
    {
        m_classes.push_back(class_);
    }

    void addFunction(const Function& function)
    {
        m_functions.push_back(function);
    }

    void addGlobalVariable(const VariableValue& variable)
    {
        m_globalVariables.push_back(variable);
    }

private:
    std::string m_name;
    bool m_isHeader;
    std::vector<std::string> m_includes;
    std::vector<std::string> m_macros;
    std::vector<std::string> m_nestedNamespaces;
    std::vector<Class> m_classes;
    std::vector<Function> m_functions;
    std::vector<VariableValue> m_globalVariables;
}; // class SourceFile

}

