#include "CodeGen/ProtoFileWriter.h"

namespace vazgen {

ProtoFileWriter::ProtoFileWriter(const std::string& name,
                                 const ProtoFile& protoFile)
    : m_writer(name)
    , m_protoFile(protoFile)
                                    
{
}

void ProtoFileWriter::write()
{
}

void ProtoFileWriter::beginProtoFile(const ProtoFile& file)
{
}

void ProtoFileWriter::writeProtoMessage(const ProtoMessage& msg)
{
}

void ProtoFileWriter::writeProtoService(const ProtoMessage& msg)
{
}

void ProtoFileWriter::writeProtoVersion(const std::string& version)
{
}

void ProtoFileWriter::writeProtoPackage(const std::string& package)
{
}

void ProtoFileWriter::writeProtoImports(const ProtoFile::Imports& imports)
{
}

void ProtoFileWriter::writeProtoImport(const ProtoFile::Import& import)
{
}

void ProtoFileWriter::beginProtoMessage(const ProtoMessage& msg)
{
}

void ProtoFileWriter::writeMessageFields(const ProtoMessage::Fields& fields)
{
}

void ProtoFileWriter::writeMessageField(const ProtoMessage::Field& fields)
{
}

void ProtoFileWriter::beginProtoService(const ProtoService& service)
{
}

void ProtoFileWriter::writeServiceRpcs(const ProtoService::RPCs& rpcs)
{
}

void ProtoFileWriter::writeServiceRpc(const ProtoService::RPC& rpcs)
{
}

} // namespace vazgen

