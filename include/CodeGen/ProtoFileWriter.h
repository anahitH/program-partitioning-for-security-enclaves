#pragma once

#include "CodeGen/FileWriter.h"
#include "CodeGen/ProtoFile.h"

namespace vazgen {

class ProtoFileWriter
{
public:
    ProtoFileWriter(const std::string& name, const ProtoFile& protoFile);

    ProtoFileWriter(const ProtoFileWriter& writer) = delete;
    ProtoFileWriter(ProtoFileWriter&& writer) = delete;
    ProtoFileWriter& operator =(const ProtoFileWriter& ) = delete;
    ProtoFileWriter& operator =(ProtoFileWriter&& ) = delete;

public:
    void write();

private:
    void beginProtoFile(const ProtoFile& file);
    void writeProtoMessage(const ProtoMessage& msg);
    void writeProtoService(const ProtoMessage& msg);
    void writeProtoVersion(const std::string& version);
    void writeProtoPackage(const std::string& package);
    void writeProtoImports(const ProtoFile::Imports& imports);
    void writeProtoImport(const ProtoFile::Import& import);
    void beginProtoMessage(const ProtoMessage& msg);
    void writeMessageFields(const ProtoMessage::Fields& fields);
    void writeMessageField(const ProtoMessage::Field& fields);
    void beginProtoService(const ProtoService& service);
    void writeServiceRpcs(const ProtoService::RPCs& rpcs);
    void writeServiceRpc(const ProtoService::RPC& rpcs);

private:
    const ProtoFile& m_protoFile;
    FileWriter m_writer;
}; // class ProtoFileWriter

} // namespace vazgen

