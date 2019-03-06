#include "CodeGen/UtilsGenerator.h"

#include <sstream>
#include <iostream>

namespace vazgen {

UtilsGenerator::UtilsGenerator(const std::string& name, const ProtoFile& protoFile)
    : m_utilsClass(name + "Utils")
    , m_protoFile(protoFile)
{
    m_header.setHeader(true);
    m_header.setName(name + "Utils.h");
    m_header.addMacro("#pragma once");

    m_source.setName(name + "Utils.cpp");
    m_source.setHeader(false);
    m_source.addInclude("\"" + m_header.getName() + "\"\n");
    m_source.addInclude("<cstdint>");
}

void UtilsGenerator::setDataNamespace(const std::string& namespace_)
{
    m_dataNamespace = namespace_;
}

void UtilsGenerator::addInclude(const std::string& include)
{
    m_header.addInclude(include);
}

void UtilsGenerator::generate()
{
    for (const auto& [name, service] : m_protoFile.getServices()) {
        for (const auto& rpc : service.getRPCs()) {
            (m_setters == INPUT) ? generateUtilMarshalFunctionsForMessage(rpc.m_input) : generateUtilMarshalFunctionsForMessage(rpc.m_output);
            (m_getters == INPUT) ? generateUtilUnmarshalFunctionsForField(rpc.m_input) : generateUtilUnmarshalFunctionsForField(rpc.m_output);
        }
    }
    m_header.addClass(m_utilsClass);
    m_source.addClass(m_utilsClass);
}

void UtilsGenerator::generateUtilMarshalFunctionsForMessage(const ProtoMessage& msg)
{
    for (const auto& field : msg.getFields()) {
        generateUtilMarshalFunctionsForField(field, msg);
    }
}

void UtilsGenerator::generateUtilUnmarshalFunctionsForField(const ProtoMessage& msg)
{
    for (const auto& field : msg.getFields()) {
        generateUtilUnmarshalFunctionsForField(field, msg);
    }
}

Function UtilsGenerator::generateUtilUnmarshalFunctionsForField(const ProtoMessage::Field& field,
                                                                const ProtoMessage& msg)
{
    Function getF("unmarshal_" + field.m_name);
    getF.setIsStatic(true);
    getF.setReturnType(Type{"void", "", false, false});
    Variable fieldParam;
    fieldParam.m_name = field.m_name;
    fieldParam.m_type = {field.m_Ctype, "", true, false};
    getF.addParam(fieldParam);
    Variable msgParam;
    msgParam.m_name = m_getters == INPUT ? "input" : "output";
    msgParam.m_type = Type{m_dataNamespace + "::" + msg.getName(), "const", true, false};
    getF.addParam(msgParam);
    getF.addBody("// unmarshaling");
    if (field.m_isRepeated) {
        getF.addBody("// TODO: fill in array unmarshaling");
        m_utilsClass.addMemberFunction(Class::PUBLIC, getF);
        return getF;
    }
    if (!m_protoFile.hasMessage(field.m_type)) {
        std::stringstream getStrm;
        getStrm << fieldParam.m_name << " = "
                << msgParam.m_name << "->" << fieldParam.m_name << "();";
        getF.addBody(getStrm.str());
    } else {
        const auto& fieldMsg = m_protoFile.getMessage(field.m_type);
        for (const auto& f : fieldMsg.getFields()) {
            std::stringstream getStrm;
            if (m_protoFile.hasMessage(f.m_type)) {
                const auto& F = generateUtilUnmarshalFunctionsForField(f, fieldMsg);
                getStrm << F.getCallAsString({field.m_name + "->" + f.m_name, "*" + field.m_name});
            } else if (f.m_isRepeated) {
                getStrm << "// TODO: fill in array unmarshaling;";
            } else {
                getStrm << field.m_name << "->" << f.m_name << " = " << msgParam.m_name << "->" << fieldParam.m_name << "()->" << f.m_name << "();";
            }
            getF.addBody(getStrm.str());
        }
    }
    m_utilsClass.addMemberFunction(Class::PUBLIC, getF);
    return getF;
}

Function UtilsGenerator::generateUtilMarshalFunctionsForField(const ProtoMessage::Field& field,
                                                              const ProtoMessage& msg)
{
    Function setF("marshal_" + field.m_name);
    setF.setIsStatic(true);
    setF.setReturnType(Type{"void", "", false, false});
    Variable fieldParam;
    fieldParam.m_name = field.m_name;
    fieldParam.m_type = {field.m_Ctype, "const", true, false};
    setF.addParam(fieldParam);
    Variable msgParam;
    msgParam.m_name = m_setters == INPUT ? "input" : "output";
    msgParam.m_type = Type{m_dataNamespace + "::" + msg.getName(), "", true, false};
    setF.addParam(msgParam);
    setF.addBody("// marshaling");

    if (field.m_isRepeated) {
        setF.addBody("// TODO: fill in array marshaling");
        m_utilsClass.addMemberFunction(Class::PUBLIC, setF);
        return setF;
    }
   if (!m_protoFile.hasMessage(field.m_type)) {
        std::stringstream setStrm;
        setStrm << msgParam.m_name << "->marshal_" << fieldParam.m_name << "(" << fieldParam.m_name << ");";
        setF.addBody(setStrm.str());
    } else {
        const auto& fieldMsg = m_protoFile.getMessage(field.m_type);
        for (const auto& f : fieldMsg.getFields()) {
            std::stringstream setStrm;
            if (m_protoFile.hasMessage(f.m_type)) {
                const auto& F = generateUtilMarshalFunctionsForField(f, fieldMsg);
                setStrm << F.getCallAsString({field.m_name + "->" + f.m_name, "*" + field.m_name});
            } else if (f.m_isRepeated) {
                setStrm << "// TODO: fill in array marshaling\n;";
            } else {
                setStrm << msgParam.m_name << "->mutable_" << fieldParam.m_name << "()->marshal_" << f.m_name << "(" << fieldParam.m_name << "->" << f.m_name << ");";
            }
            setF.addBody(setStrm.str());
        }
    }
    m_utilsClass.addMemberFunction(Class::PUBLIC, setF);
    return setF;
}


} // namespace vazgen

