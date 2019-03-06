#pragma once

#include "CodeGen/Class.h"
#include "CodeGen/ProtoFile.h"
#include "CodeGen/ProtoMessage.h"
#include "CodeGen/SourceFile.h"

namespace vazgen {

class UtilsGenerator
{
public:
    enum GenFor
    {
        INPUT,
        OUTPUT
    };

public:
    UtilsGenerator(const std::string& name, const ProtoFile& protoFile);

public:
    void setSettersFor(GenFor setFor)
    {
        m_setters = setFor;
    }

    void setGettersFor(GenFor getFor)
    {
        m_getters = getFor;
    }

    const SourceFile& getHeader() const
    {
        return m_header;
    }

    const SourceFile& getSource() const
    {
        return m_source;
    }

    void setDataNamespace(const std::string& namespace_);
    void addInclude(const std::string& include);

    void generate();

private:
    void generateUtilMarshalFunctionsForMessage(const ProtoMessage& msg);
    void generateUtilUnmarshalFunctionsForField(const ProtoMessage& msg);
    Function generateUtilUnmarshalFunctionsForField(const ProtoMessage::Field& field,
                                                    const ProtoMessage& msg);
    Function generateUtilMarshalFunctionsForField(const ProtoMessage::Field& field,
                                                  const ProtoMessage& msg);

private:
    std::string m_dataNamespace;
    Class m_utilsClass;
    SourceFile m_header;
    SourceFile m_source;
    const ProtoFile& m_protoFile;
    GenFor m_setters;
    GenFor m_getters;
}; // class UtilsGenerator

}; // namespace vazgen

