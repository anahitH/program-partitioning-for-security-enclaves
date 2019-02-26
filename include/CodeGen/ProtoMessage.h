#pragma once

#include <vector>
#include <string>

namespace vazgen {

class ProtoMessage
{
public:
    struct Field
    {
        std::string m_attribute;
        std::string m_type;
        std::string m_name;
        int m_number = 1;
        bool m_isPtr = false;
        bool m_isRepeated;
        // type if C code
        std::string m_Ctype;
    };

    struct Enum
    {
        std::string m_name;
        using Value = std::pair<std::string, int>;
        std::vector<Value> m_values;
    };
    
    using Fields = std::vector<Field>;
    using Enums = std::vector<Enum>;

public:
    ProtoMessage() = default;
    ProtoMessage(const std::string& name)
        : m_name(name)
    {
    }

    ProtoMessage(const ProtoMessage& ) = default;
    ProtoMessage(ProtoMessage&& ) = default;
    ProtoMessage& operator =(const ProtoMessage& ) = default;
    ProtoMessage& operator =(ProtoMessage&& ) = default;

public:
    const std::string& getName() const
    {
        return m_name;
    }

    const std::string& getAsString() const
    {
        return m_name;
    }

    const Fields& getFields() const
    {
        return m_fields;
    }

    const Enums& getEnums() const
    {
        return m_enums;
    }

    void setName(const std::string& name)
    {
        m_name = name;
    }

    void setFields(const Fields& fields)
    {
        m_fields = fields;
    }

    void setFields(Fields&& fields)
    {
        m_fields = std::move(fields);
    }

    void setEnums(const Enums& enums)
    {
        m_enums = enums;
    }

    void setFields(Enums&& enums)
    {
        m_enums = std::move(enums);
    }

    void addField(const Field& f)
    {
        m_fields.push_back(f);
    }

    void addField(Field&& f)
    {
        m_fields.push_back(std::move(f));
    }

    void addEnum(const Enum& e)
    {
        m_enums.push_back(e);
    }

    void addEnum(Enum&& e)
    {
        m_enums.push_back(std::move(e));
    }

private:
    std::string m_name;
    Fields m_fields;
    Enums m_enums;
}; // class ProtoMessage

} // namespace vazgen

