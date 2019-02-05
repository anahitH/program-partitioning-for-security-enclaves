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
    beginProtoFile();
    writeProtoImports(m_protoFile.getImports());
    writeProtoMessages(m_protoFile.getMessages());
    writeProtoServices(m_protoFile.getServices());
}

void ProtoFileWriter::beginProtoFile()
{
    writeProtoVersion(m_protoFile.getVersion());
    writeProtoPackage(m_protoFile.getPackage());
}

void ProtoFileWriter::writeProtoVersion(const std::string& version)
{
    m_writer.write("syntax = \"" + version + "\";");
}

void ProtoFileWriter::writeProtoPackage(const std::string& package)
{
    m_writer.write("package " + package + ";");
}

void ProtoFileWriter::writeProtoImports(const ProtoFile::Imports& imports)
{
    for (const auto& import : imports) {
        writeProtoImport(import);
    }
}

void ProtoFileWriter::writeProtoImport(const ProtoFile::Import& import)
{
    m_writer.write("import \"" + import.m_name + "\";");
}


void ProtoFileWriter::writeProtoMessages(const ProtoFile::Messages& msgs)
{
    for (const auto& msg : msgs) {
        writeProtoMessage(msg);
    }
}

void ProtoFileWriter::writeProtoMessage(const ProtoMessage& msg)
{
    beginProtoMessage(msg);
    writeMessageFields(msg.getFields());
    writeMessageEnums(msg.getEnums());
    endProtoMessage();
}

void ProtoFileWriter::beginProtoMessage(const ProtoMessage& msg)
{
    m_writer.write("message " + msg.getName() + " {\n");
}

void ProtoFileWriter::writeMessageFields(const ProtoMessage::Fields& fields)
{
    for (const auto& field : fields) {
        writeMessageField(field);
    }
}

void ProtoFileWriter::writeMessageField(const ProtoMessage::Field& field)
{
    m_writer.write(field.m_attribute + " ");
    m_writer.write(field.m_type + " ");
    m_writer.write(field.m_name + " ");
    m_writer.write(" = ");
    m_writer.write(field.m_number);
    m_writer.write(";");
}

void ProtoFileWriter::writeMessageEnums(const ProtoMessage::Enums& enums)
{
    for (const auto& en : enums) {
        writeMessageEnum(en);
    }
}

void ProtoFileWriter::writeMessageEnum(const ProtoMessage::Enum& enum_)
{
    m_writer.write("enum " + enum_.m_name + " {");
    for (const auto& val : enum_.m_values) {
        if (val.second == -1) {
            m_writer.write(val.first + ";");
        } else {
            m_writer.write(val.first + " = " + std::to_string(val.second) + ";");
        }
    }
    m_writer.write("}");
}

void ProtoFileWriter::endProtoMessage()
{
    m_writer.write("}");
}

void ProtoFileWriter::writeProtoServices(const ProtoFile::Services& services)
{
    for (const auto& service : services) {
        writeProtoService(service);
    }
}

void ProtoFileWriter::writeProtoService(const ProtoService& service)
{
    beginProtoService(service);
    writeServiceRpcs(service.getRPCs());
    endProtoService();
}

void ProtoFileWriter::beginProtoService(const ProtoService& service)
{
    m_writer.write("service " + service.getName() + " {");
}

void ProtoFileWriter::endProtoService()
{
    m_writer.write("}");
}

void ProtoFileWriter::writeServiceRpcs(const ProtoService::RPCs& rpcs)
{
    for (const auto& rpc : rpcs) {
        writeServiceRpc(rpc);
    }
}

void ProtoFileWriter::writeServiceRpc(const ProtoService::RPC& rpc)
{
    m_writer.write("rpc ");
    m_writer.write(rpc.m_name + " ");
    m_writer.write("(" + rpc.m_input.getName() + ") ");
    m_writer.write("return (" + rpc.m_output.getName() + ") {}");
}

} // namespace vazgen

