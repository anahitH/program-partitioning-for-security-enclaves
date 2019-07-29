#pragma once

#include <string>
#include <vector>

namespace vazgen {

struct Type
{
    std::string m_name;
    std::string m_qualifier;
    bool m_isPtr;
    bool m_isArray;

    std::string getAsString() const;
};

struct Variable
{
    Type m_type;
    std::string m_name;

    std::string getAsString() const;
};

class Function
{
public:
    using Params = std::vector<Variable>;
    using Instruction = std::string;
    using Body = std::vector<Instruction>;
    
public:
    Function() = default;
    Function(std::string name)
        : m_name(name)
        , m_isStatic(false)
        , m_isExtern(false)
        , m_declPrefix("")
    {
    }
    
    std::string getName() const
    {
        return m_name;
    }

    bool isStatic() const
    {
        return m_isStatic;
    }

    bool isExtern() const
    {
        return m_isExtern;
    }

    const std::string& getDeclPrefix() const
    {
        return m_declPrefix;
    }

    Type getReturnType() const
    {
        return m_returnType;
    }

    bool isVoidReturn() const
    {
        return m_returnType.m_name == "void";
    }

    const Params& getParams() const
    {
        return m_params;
    }

    const std::string& getAccessModifier() const
    {
        return m_accessModifier;
    }

    void setName(const std::string name)
    {
        m_name = name;
    }

    void setIsStatic(bool isStatic)
    {
        m_isStatic = isStatic;
    }

    void setIsExtern(bool isExtern)
    {
        m_isExtern = isExtern;
    }

    void setDeclPrefix(const std::string& defPrefix)
    {
        m_declPrefix = defPrefix;
    }

    void setReturnType(const Type& type)
    {
        m_returnType = type;
    }

    void setParams(const Params& params)
    {
        m_params = params;
    }

    void addParam(const Variable& param)
    {
        m_params.push_back(param);
    }

    void setAccessModifier(const std::string accessModifier)
    {
        m_accessModifier = accessModifier;
    }

    void setBody(const Body& body)
    {
        m_body = body;
    }

    void addBody(const Instruction& instr)
    {
        m_body.push_back(instr);
    }

    std::string getDeclarationAsString() const;
    std::string getDefinitionAsString() const;
    std::string getDefinitionAsString(const std::string& className) const;
    std::string getCallAsString(const std::vector<Variable>& arguments) const;

private:
    std::string m_name;
    bool m_isStatic;
    bool m_isExtern;
    std::string m_declPrefix;
    Type m_returnType;
    Params m_params;
    std::string m_accessModifier;
    Body m_body;
}; // class Funciton

} // namespace vazgen

