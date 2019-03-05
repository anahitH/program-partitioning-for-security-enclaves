#include "CodeGen/ServiceImplGenerator.h"
#include "CodeGen/UtilsGenerator.h"

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
    UtilsGenerator utilsGenerator(m_protoFile.getName(), m_protoFile);
    utilsGenerator.setSettersFor(UtilsGenerator::OUTPUT);
    utilsGenerator.setGettersFor(UtilsGenerator::INPUT);
    utilsGenerator.addInclude("\"" + m_protoFile.getPackage() + ".grpc.pb.h\"");
    utilsGenerator.setNamespace(m_protoFile.getPackage());
    utilsGenerator.generate();
    m_serviceFiles[m_protoFile.getName() + "Utils"].first = utilsGenerator.getHeader();
    m_serviceFiles[m_protoFile.getName() + "Utils"].second = utilsGenerator.getSource();
}

} // namespace vazgen

