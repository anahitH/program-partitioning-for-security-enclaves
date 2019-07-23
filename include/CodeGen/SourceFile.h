#pragma once

#include "CodeGen/Class.h"
#include <memory>
#include <vector>

namespace vazgen {

class SourceScope
{
public:
    using ScopeType = std::shared_ptr<SourceScope>;
    using VariableValue = std::pair<Variable, std::string>;

public:
    SourceScope() = default;
    SourceScope(std::string name, bool isNamespace = true)
        : m_name(name)
        , m_isNamespace(isNamespace)
    {
    }

public:
    const std::string& getName() const
    {
        return m_name;
    }

    const bool isNamespace() const
    {
        return m_isNamespace;
    }
    
    const std::vector<ScopeType>& getSubScopes() const
    {
        return m_subScopes;
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

    void setSubScopes(const std::vector<ScopeType>& subScopes)
    {
        m_subScopes = subScopes;
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

    void addSubscope(const ScopeType& scope)
    {
        m_subScopes.push_back(scope);
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


protected:
    std::string m_name;
    bool m_isNamespace;
    std::vector<ScopeType> m_subScopes;
    std::vector<Class> m_classes;
    std::vector<Function> m_functions;
    std::vector<VariableValue> m_globalVariables;
}; // class SourceScope


class SourceFile : public SourceScope
{
public:
    SourceFile() = default;
    SourceFile(std::string name, bool isHeader)
        : SourceScope(name)
        , m_isHeader(isHeader)
    {
    }

public:
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

    const std::vector<ScopeType>& getNamespaces() const
    {
        return getSubScopes();
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

    void addInclude(const std::string& include)
    {
        m_includes.push_back(include);
    }

    void addMacro(const std::string& macro)
    {
        m_macros.push_back(macro);
    }

private:
    std::string m_name;
    bool m_isHeader;
    std::vector<std::string> m_includes;
    std::vector<std::string> m_macros;
}; // class SourceFile

}

