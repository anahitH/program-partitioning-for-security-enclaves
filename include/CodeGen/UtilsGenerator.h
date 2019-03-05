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

    void setNamespace(const std::string& namespace_);
    void addInclude(const std::string& include);

    void generate();

private:
    void generateUtilSetFunctionsForMessage(const ProtoMessage& msg);
    void generateUtilGetFunctionsForMessage(const ProtoMessage& msg);
    void generateUtilGetFunctionsForField(const ProtoMessage::Field& field,
                                          const ProtoMessage& msg);
    void generateUtilSetFunctionsForField(const ProtoMessage::Field& field,
                                          const ProtoMessage& msg);

private:
    Class m_utilsClass;
    SourceFile m_header;
    SourceFile m_source;
    const ProtoFile& m_protoFile;
    GenFor m_setters;
    GenFor m_getters;
}; // class UtilsGenerator

}; // namespace vazgen

