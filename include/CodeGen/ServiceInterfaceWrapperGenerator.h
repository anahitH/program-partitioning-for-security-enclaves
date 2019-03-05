#pragma once

#include "CodeGen/SourceFile.h"
#include "CodeGen/ProtoFile.h"

namespace vazgen {

class Function;

class ServiceInterfaceWrapperGenerator
{
public:
    ServiceInterfaceWrapperGenerator(const ProtoFile& serviceProtoFile,
                                     const ProtoFile& dataProtoFile);

public:
    void generate();

    const std::unordered_map<std::string, SourceFile>& getWrapperFiles() const
    {
        return m_generatedFiles;
    }

private:
    Function generateWrapperFunction(const ProtoService::RPC& rpc);
    void generateFunctionBody(Function& wrapperF, const ProtoService::RPC& rpc);
    void generateUtilFile();

private:
    const ProtoFile& m_serviceProtoFile;
    const ProtoFile& m_serviceDataFile;
    std::unordered_map<std::string, SourceFile> m_generatedFiles;
}; // class ServiceInterfaceWrapperGenerator

} // namespace vazgen


