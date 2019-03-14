#include "CodeGen/Class.h"

#include <cassert>
#include <sstream>

namespace vazgen {

std::string accessSpecifierToString(Class::AccessSpecifier access)
{
    switch (access) {
    case Class::PUBLIC:
        return "public";
    case Class::PRIVATE:
        return "private";
    case Class::PROTECTED:
        return "protected";
    }
    assert(false);
    return "";
}

std::string Class::getClassDeclarationAsString() const
{
    std::stringstream declStr;
    declStr << "class " << m_name;
    if (m_parents.empty()) {
        return declStr.str();
    }
    int i = 0;
    for (const auto& [access, parent] : m_parents) {
        if (i == 0) {
            declStr << " : ";
        } else if (i != m_parents.size() - 1) {
            declStr << ",";
        }
        declStr  << accessSpecifierToString(access)
                 << " " << parent << "\n";
        ++i;
    }
    return declStr.str();
}

std::string Class::getClassDefinitionAsString() const
{
    // TODO: implement
    return std::string();
}

}

