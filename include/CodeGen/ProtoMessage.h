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
        std::string m_number;
    };
    
    using Fields = std::vector<Field>;

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

    void addField(const Field& f)
    {
        m_fields.push_back(f);
    }

    void addField(Field&& f)
    {
        m_fields.push_back(std::move(f));
    }


private:
    std::string m_name;
    Fields m_fields;
}; // class ProtoMessage

} // namespace vazgen

