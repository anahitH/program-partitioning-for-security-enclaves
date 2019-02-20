#pragma once

#include "CodeGen/Function.h"

#include <unordered_map>

namespace vazgen {

class Class
{
public:
    enum AccessSpecifier
    {
        PUBLIC,
        PRIVATE,
        PROTECTED
    };

public:
    using Parents = std::unordered_map<AccessSpecifier, std::string>;
    using MemberFunctions = std::unordered_map<AccessSpecifier, Function>;
    using Members = std::unordered_map<AccessSpecifier, Variable>;

public:
    Class() = default;
    Class(const std::string name)
        : m_name(name)
    {
    }

public:
    const std::string& getName() const
    {
        return m_name;
    }

    const Parents& getParents() const
    {
        return m_parents;
    }

    const MemberFunctions& getMemberFunctions() const
    {
        return m_memberFunctions;
    }

    const Members& getMembers() const
    {
        return m_members;
    }

    void setName(const std::string& name)
    {
        m_name = name;
    }

    void setParents(const Parents& parents)
    {
        m_parents =  parents;
    }

    void setMemberFunctions(const MemberFunctions& functions)
    {
        m_memberFunctions =  functions;
    }
    
    void setMembers(const Members& members)
    {
        m_members = members;
    }

    void addParent(AccessSpecifier accessSpec, const std::string& parent)
    {
        m_parents.insert(std::make_pair(accessSpec, parent));
    }

    void addMemberFunction(AccessSpecifier accessSpec, const Function& f)
    {
        m_memberFunctions.insert(std::make_pair(accessSpec, f));
    }

    void addMember(AccessSpecifier accessSpec, const Variable& v)
    {
        m_members.insert(std::make_pair(accessSpec, v));
    }

    std::string getClassDeclarationAsString() const;
    std::string getClassDefinitionAsString() const;

private:
    std::string m_name;
    Parents m_parents;
    MemberFunctions m_memberFunctions;
    Members m_members;
}; // class Class

} // namespace vazgen

