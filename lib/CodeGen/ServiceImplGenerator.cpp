#include "CodeGen/ServiceImplGenerator.h"

#include <sstream>

namespace vazgen {

void ServiceImplGenerator::generate()
{
    for (const auto& [name, service] : m_protoFile.getServices()) {
        generateForService(name, service);
    }
}

void ServiceImplGenerator::generateForService(const std::string& serviceName,
                                              const ProtoService& service)
{
    auto& [header, source] = m_serviceFiles[serviceName];
    Class serviceClass(serviceName + "Impl");
    serviceClass.addParent(Class::PUBLIC, serviceName + "::Service");
    for (const auto& rpc : service.getRPCs()) {
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
        serviceClass.addMemberFunction(Class::PUBLIC, rpcF);
        generateFunctionBody(rpcF, rpc);
    }
    header.setName(serviceName + ".h");
    header.setHeader(true);
    header.addMacro("#pragma once");
    header.addInclude("\"" + m_protoFile.getPackage() + ".grpc.pb.h\"");
    header.addInclude("Utils.h");
    header.addInclude("\"include/grpcpp/grpcpp.h\"");
    header.addInclude("\"include/grpcpp/server.h\"");
    header.setNamespace(m_protoFile.getPackage());
    header.addClass(serviceClass);

    source.setName(serviceName + ".cpp");
    source.setHeader(false);
    source.addInclude("\"" + header.getName() + "\n");
    source.setNamespace(m_protoFile.getPackage());
    source.addClass(serviceClass);
}

void ServiceImplGenerator::generateFunctionBody(Function& F,
                                                const ProtoService::RPC& rpc)
{
    std::stringstream body;
    Function actualF(rpc.m_name);
    for (const auto& field : rpc.m_input.getFields()) {
        F.addBody(field.m_type + " " + field.m_name);
        std::stringstream fCall;
        fCall << "Utils::get_"
              << field.m_name << "(&"
              << field.m_name << ","
              << rpc.m_input.getName() << ");";
        F.addBody(fCall.str());
    }
    // TODO: insert real call
    for (const auto& field : rpc.m_output.getFields()) {
        F.addBody(field.m_type + " " + field.m_name);
        std::stringstream fCall;
        fCall << "Utils::set_"
              << field.m_name << "(&"
              << field.m_name << ","
              << rpc.m_output.getName() << ");";
        F.addBody(fCall.str());
    }
    F.addBody("return ::grpc::Status::OK;");
}

}

