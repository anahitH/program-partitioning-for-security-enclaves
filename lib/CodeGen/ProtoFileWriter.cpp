#include "CodeGen/ProtoFileWriter.h"

#include <sstream>

namespace vazgen {

ProtoFileWriter::ProtoFileWriter(const std::string& name,
                                 const ProtoFile& protoFile)
    : m_writer(name)
    , m_protoFile(protoFile)
    , m_indent(0)
                                    
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
    m_writer.write("syntax = \"" + version + "\";", m_indent);
}

void ProtoFileWriter::writeProtoPackage(const std::string& package)
{
    m_writer.write("package " + package + ";", m_indent);
}

void ProtoFileWriter::writeProtoImports(const ProtoFile::Imports& imports)
{
    for (const auto& import : imports) {
        writeProtoImport(import);
    }
}

void ProtoFileWriter::writeProtoImport(const ProtoFile::Import& import)
{
    m_writer.write("import \"" + import.m_name + "\";", m_indent);
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
    writeMessageEnums(msg.getEnums());
    writeMessageFields(msg.getFields());
    endProtoMessage();
}

void ProtoFileWriter::beginProtoMessage(const ProtoMessage& msg)
{
    m_writer.write("message " + msg.getName() + " {", m_indent);
}

void ProtoFileWriter::writeMessageFields(const ProtoMessage::Fields& fields)
{
    ++m_indent;
    for (const auto& field : fields) {
        writeMessageField(field);
    }
    --m_indent;
}

void ProtoFileWriter::writeMessageField(const ProtoMessage::Field& field)
{
    std::stringstream strStrm;
    strStrm << field.m_attribute << " "
            << field.m_type << " "
            << field.m_name << " "
            << "=" << field.m_number << ";";
    m_writer.write(strStrm.str(), m_indent);
}

void ProtoFileWriter::writeMessageEnums(const ProtoMessage::Enums& enums)
{
    ++m_indent;
    for (const auto& en : enums) {
        writeMessageEnum(en);
    }
    --m_indent;
}

void ProtoFileWriter::writeMessageEnum(const ProtoMessage::Enum& enum_)
{
    std::stringstream strStrm;
    m_writer.write("enum " + enum_.m_name + " {", m_indent);
    ++m_indent;
    for (const auto& val : enum_.m_values) {
        if (val.second == -1) {
            m_writer.write(val.first + ";", m_indent);
        } else {
            m_writer.write(val.first + " = " + std::to_string(val.second) + ";", m_indent);
        }
    }
    --m_indent;
    m_writer.write("}");
}

void ProtoFileWriter::endProtoMessage()
{
    m_writer.write("}\n", m_indent);
}

void ProtoFileWriter::writeProtoServices(const ProtoFile::Services& services)
{
    for (const auto& [name, service] : services) {
        writeProtoService(service);
    }
}

void ProtoFileWriter::writeProtoService(const ProtoService& service)
{
    beginProtoService(service);
    ++m_indent;
    writeServiceRpcs(service.getRPCs());
    --m_indent;
    endProtoService();
}

void ProtoFileWriter::beginProtoService(const ProtoService& service)
{
    m_writer.write("service " + service.getName() + " {", m_indent);
}

void ProtoFileWriter::endProtoService()
{
    m_writer.write("}", m_indent);
}

void ProtoFileWriter::writeServiceRpcs(const ProtoService::RPCs& rpcs)
{
    for (const auto& rpc : rpcs) {
        writeServiceRpc(rpc);
    }
}

void ProtoFileWriter::writeServiceRpc(const ProtoService::RPC& rpc)
{
    std::stringstream strStrm;
    strStrm << "rpc " << rpc.m_name << "( "
            << rpc.m_input.getName() << ") "
            << " returns (" << rpc.m_output.getName() << ") {}";
    m_writer.write(strStrm.str(), m_indent);
}

} // namespace vazgen

