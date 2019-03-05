#pragma once

#include "CodeGen/SourceFile.h"
#include "CodeGen/ProtoFile.h"

namespace vazgen {

class ServiceImplGenerator
{
public:
    using HeaderSrcFiles = std::pair<SourceFile, SourceFile>;

public:
    ServiceImplGenerator(const ProtoFile& protoFile)
        : m_protoFile(protoFile)
    {
    }

    void generate();

    const std::unordered_map<std::string, HeaderSrcFiles>& getServiceImplFiles() const
    {
        return m_serviceFiles;
    }

private:
    void generateForService(const std::string& serviceName, const ProtoService& service);
    void generateFunctionBody(Function& F, const ProtoService::RPC& rpc);
    void generateUtilsClass();

private:
    const ProtoFile& m_protoFile;
    std::unordered_map<std::string, HeaderSrcFiles> m_serviceFiles;
}; // class ServiceImplGenerator

}

