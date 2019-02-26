#include "CodeGen/ServiceImplGenerator.h"

#include <sstream>
#include <iostream>

namespace vazgen {

void ServiceImplGenerator::generate()
{
    for (const auto& [name, service] : m_protoFile.getServices()) {
        generateForService(name, service);
    }
    generateUtilsClass();
}

void ServiceImplGenerator::generateForService(const std::string& serviceName,
                                              const ProtoService& service)
{
    auto& [header, source] = m_serviceFiles[serviceName];
    Class serviceClass(serviceName + "Impl");
    serviceClass.addParent(Class::PUBLIC, serviceName + "::Service");
    std::cout << "Generate class for service " << serviceName << "\n";
    for (const auto& rpc : service.getRPCs()) {
        std::cout << "Generate function " << rpc.m_name << "\n";
        Function rpcF(rpc.m_name);
        rpcF.setReturnType({"::grpc::Status", "", false, false});
        Variable serverCtxParam;
        serverCtxParam.m_type = {"::grpc::ServerContext", "", true, false};
        serverCtxParam.m_name = "context";
        rpcF.addParam(serverCtxParam);
        Variable inputParam;
        inputParam.m_name = "input";
        inputParam.m_type = {rpc.m_input.getName(), "const", true, false};
        rpcF.addParam(inputParam);
        Variable outParam;
        outParam.m_name = "output";
        outParam.m_type = {rpc.m_output.getName(), "", true, false};
        rpcF.addParam(outParam);
        generateFunctionBody(rpcF, rpc);
        serviceClass.addMemberFunction(Class::PUBLIC, rpcF);
    }
    header.setName(serviceName + ".h");
    header.setHeader(true);
    header.addMacro("#pragma once");
    header.addInclude("\"" + m_protoFile.getPackage() + ".grpc.pb.h\"");
    header.addInclude("\"include/grpcpp/grpcpp.h\"");
    header.addInclude("\"include/grpcpp/server.h\"");
    header.setNamespace(m_protoFile.getPackage());
    header.addClass(serviceClass);

    source.setName(serviceName + ".cpp");
    source.setHeader(false);
    source.addInclude("\"" + header.getName() + "\"\n");
    source.addInclude("\"Utils.h\"");
    source.addInclude("<cstdint>");
    source.setNamespace(m_protoFile.getPackage());
    source.addClass(serviceClass);
}

void ServiceImplGenerator::generateFunctionBody(Function& F,
                                                const ProtoService::RPC& rpc)
{
    std::stringstream body;
    Function actualF(rpc.m_name);
    std::vector<std::string> callArgs;
    for (const auto& field : rpc.m_input.getFields()) {
        std::stringstream instrStrm;
        instrStrm << field.m_Ctype;
        if (field.m_isRepeated) {
            instrStrm << "[]";
        }
        instrStrm << " " << field.m_name << ";";
        F.addBody(instrStrm.str());
        std::stringstream fCall;
        fCall << "Utils::get_"
              << field.m_name << "(&"
              << field.m_name << ", "
              << "input);";
        F.addBody(fCall.str());
        Type type = {field.m_Ctype, "", field.m_isPtr, field.m_isRepeated};
        actualF.addParam(Variable{type, field.m_name});
        callArgs.push_back(field.m_name);
    }
    // insert real call
    F.addBody("// real call");
    // if call returns a value
    bool hasReturnValue = false;
    if (rpc.m_output.getFields().size() != rpc.m_input.getFields().size()) {
        hasReturnValue = true;
        const auto& returnField = rpc.m_output.getFields().back();
        F.addBody(returnField.m_Ctype + " " + returnField.m_name + " = " + actualF.getCallAsString(callArgs));
    } else {
        F.addBody(actualF.getCallAsString(callArgs));
    }
    int i = 0;
    for (const auto& field : rpc.m_output.getFields()) {
        if (hasReturnValue && i++ != rpc.m_output.getFields().size() - 1) {
            F.addBody(field.m_Ctype + " " + field.m_name + ";");
        }
        std::stringstream fCall;
        fCall << "Utils::set_"
              << field.m_name << "(&"
              << field.m_name << ", "
              << "output);";
        F.addBody(fCall.str());
    }
    // TODO: set for return value
    F.addBody("return ::grpc::Status::OK;");
}

void ServiceImplGenerator::generateUtilsClass()
{
    Class utils("ServiceUtils");
    for (const auto& [name, service] : m_protoFile.getServices()) {
        for (const auto& rpc : service.getRPCs()) {
            generateUtilGetFunctionsForMessage(rpc.m_input, utils);
            generateUtilSetFunctionsForMessage(rpc.m_output, utils);
        }
    }
    const auto& name = m_protoFile.getPackage();
    auto& [header, source] = m_serviceFiles[name + "Utils"];
    header.setName(name + "Utils.h");
    header.setHeader(true);
    header.addMacro("#pragma once");
    header.addInclude("\"" + m_protoFile.getPackage() + ".grpc.pb.h\"");
    header.setNamespace(m_protoFile.getPackage());
    header.addClass(utils);

    source.setName(name + "Utils.cpp");
    source.setHeader(false);
    source.addInclude("\"" + header.getName() + "\"\n");
    source.setNamespace(m_protoFile.getPackage());
    source.addClass(utils);
}

void ServiceImplGenerator::generateUtilGetFunctionsForMessage(const ProtoMessage& msg,
                                                              Class& utilsClass)
{
    for (const auto& field : msg.getFields()) {
        generateUtilGetFunctionsForField(field, msg, utilsClass);
    }
}

void ServiceImplGenerator::generateUtilSetFunctionsForMessage(const ProtoMessage& msg,
                                                              Class& utilsClass)
{
    for (const auto& field : msg.getFields()) {
        generateUtilSetFunctionsForField(field, msg, utilsClass);
    }
}

void ServiceImplGenerator::generateUtilGetFunctionsForField(const ProtoMessage::Field& field,
                                                            const ProtoMessage& msg,
                                                            Class& utilsClass)
{
    Function getF("get_" + field.m_name);
    getF.setIsStatic(true);
    getF.setReturnType(Type{"void", "", false, false});
    Variable fieldParam;
    fieldParam.m_name = field.m_name;
    fieldParam.m_type = {field.m_type, "", true, false};
    getF.addParam(fieldParam);
    Variable msgParam;
    msgParam.m_name = "input";
    msgParam.m_type = Type{msg.getName(), "const", true, false};
    getF.addParam(msgParam);
    getF.addBody("// TODO: implement");
    utilsClass.addMemberFunction(Class::PUBLIC, getF);

    if (m_protoFile.hasMessage(field.m_name)) {
        generateUtilGetFunctionsForMessage(m_protoFile.getMessage(field.m_name), utilsClass);
    }
}

void ServiceImplGenerator::generateUtilSetFunctionsForField(const ProtoMessage::Field& field,
                                                            const ProtoMessage& msg,
                                                            Class& utilsClass)
{
    Function setF("set_" + field.m_name);
    setF.setIsStatic(true);
    setF.setReturnType(Type{"void", "", false, false});
    Variable fieldParam;
    fieldParam.m_name = field.m_name;
    fieldParam.m_type = {field.m_type, "const", true, false};
    setF.addParam(fieldParam);
    Variable msgParam;
    msgParam.m_name = "input";
    msgParam.m_type = Type{msg.getName(), "c", true, false};
    setF.addParam(msgParam);
    setF.addBody("// TODO: implement");
    utilsClass.addMemberFunction(Class::PUBLIC, setF);

    if (m_protoFile.hasMessage(field.m_name)) {
        generateUtilGetFunctionsForMessage(m_protoFile.getMessage(field.m_name), utilsClass);
    }
}

}

