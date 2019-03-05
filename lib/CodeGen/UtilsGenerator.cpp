#include "CodeGen/UtilsGenerator.h"

#include <sstream>

namespace vazgen {

UtilsGenerator::UtilsGenerator(const std::string& name, const ProtoFile& protoFile)
    : m_utilsClass(name)
    , m_protoFile(protoFile)
{
    m_header.setHeader(true);
    m_header.setName(name + "Utils");
    m_header.addMacro("#pragma once");
    m_header.addClass(m_utilsClass);

    m_source.setName(name + "Utils");
    m_source.setHeader(false);
    m_source.addClass(m_utilsClass);
    m_source.addInclude("\"" + m_header.getName() + "\"\n");
}

void UtilsGenerator::setNamespace(const std::string& namespace_)
{
    m_header.setNamespace(namespace_);
    m_source.setNamespace(namespace_);
}

void UtilsGenerator::addInclude(const std::string& include)
{
    m_header.addInclude(include);
}

void UtilsGenerator::generate()
{
    for (const auto& [name, service] : m_protoFile.getServices()) {
        for (const auto& rpc : service.getRPCs()) {
            (m_setters == INPUT) ? generateUtilSetFunctionsForMessage(rpc.m_input) : generateUtilSetFunctionsForMessage(rpc.m_output);
            (m_getters == INPUT) ? generateUtilGetFunctionsForMessage(rpc.m_input) : generateUtilGetFunctionsForMessage(rpc.m_output);
        }
    }

}

void UtilsGenerator::generateUtilSetFunctionsForMessage(const ProtoMessage& msg)
{
    for (const auto& field : msg.getFields()) {
        generateUtilSetFunctionsForField(field, msg);
    }
}

void UtilsGenerator::generateUtilGetFunctionsForMessage(const ProtoMessage& msg)
{
    for (const auto& field : msg.getFields()) {
        generateUtilGetFunctionsForField(field, msg);
    }
}

void UtilsGenerator::generateUtilGetFunctionsForField(const ProtoMessage::Field& field,
                                                      const ProtoMessage& msg)
{
    Function getF("get_" + field.m_name);
    getF.setIsStatic(true);
    getF.setReturnType(Type{"void", "", false, false});
    Variable fieldParam;
    fieldParam.m_name = field.m_name;
    fieldParam.m_type = {field.m_type, "", true, false};
    getF.addParam(fieldParam);
    Variable msgParam;
    msgParam.m_name = m_getters == INPUT ? "input" : "output";
    msgParam.m_type = Type{msg.getName(), "const", true, false};
    getF.addParam(msgParam);
    getF.addBody("// TODO: implement");

    if (m_protoFile.hasMessage(field.m_name)) {
        generateUtilGetFunctionsForMessage(m_protoFile.getMessage(field.m_name));
        //getF.addBody(F.getCallAsString({"&" + fieldParam.m_name, msgParam.m_name}));
    } else {
        if (field.m_isRepeated) {
            getF.addBody("// TODO: fill in array unmarshaling");
        } else {
            std::stringstream getStrm;
            getStrm << fieldParam.m_name << " = "
                    << msgParam.m_name << "." << fieldParam.m_name << "()";
            getF.addBody(getStrm.str());
        }
    }
    m_utilsClass.addMemberFunction(Class::PUBLIC, getF);
}

void UtilsGenerator::generateUtilSetFunctionsForField(const ProtoMessage::Field& field,
                                                      const ProtoMessage& msg)
{
    Function setF("set_" + field.m_name);
    setF.setIsStatic(true);
    setF.setReturnType(Type{"void", "", false, false});
    Variable fieldParam;
    fieldParam.m_name = field.m_name;
    fieldParam.m_type = {field.m_type, "const", true, false};
    setF.addParam(fieldParam);
    Variable msgParam;
    msgParam.m_name = m_setters == INPUT ? "input" : "output";
    msgParam.m_type = Type{msg.getName(), "c", true, false};
    setF.addParam(msgParam);
    setF.addBody("// TODO: implement");
    m_utilsClass.addMemberFunction(Class::PUBLIC, setF);

    if (m_protoFile.hasMessage(field.m_name)) {
        generateUtilGetFunctionsForMessage(m_protoFile.getMessage(field.m_name));
        //getF.addBody(F.getCallAsString({fieldParam.m_name, "&" + msgParam.m_name}));
    } else {
        if (field.m_isRepeated) {
            setF.addBody("// TODO: fill in array marshaling");
        } else {
            std::stringstream setStrm;
            setStrm << msgParam.m_name
                    << "->set_" << fieldParam.m_name
                    << "(" << fieldParam.m_name << ");";
            setF.addBody(setStrm.str());
        }
    }
}


} // namespace vazgen

