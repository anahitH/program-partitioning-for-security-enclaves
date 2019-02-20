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
    {
    }
    
    std::string getName() const
    {
        return m_name;
    }

    Type getReturnType() const
    {
        return m_returnType;
    }

    const Params& getParams() const
    {
        return m_params;
    }

    void setName(const std::string name)
    {
        m_name = name;
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
    std::string getCallAsString(const std::vector<std::string>& arguments) const;

private:
    std::string m_name;
    Type m_returnType;
    Params m_params;
    Body m_body;
}; // class Funciton

} // namespace vazgen

