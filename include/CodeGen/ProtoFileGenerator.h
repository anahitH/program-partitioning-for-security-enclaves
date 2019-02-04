#pragma once

#include "CodeGen/ProtoFile.h"

#include <unordered_map>

namespace llvm {
class Function;
class Type;
}

namespace vazgen {

class Partition;

class ProtoFileGenerator
{
public:
    ProtoFileGenerator(const Partition& partition, const std::string& protoName);

    ProtoFileGenerator(const ProtoFileGenerator& ) = delete;
    ProtoFileGenerator(ProtoFileGenerator&& ) = delete;
    ProtoFileGenerator& operator =(const ProtoFileGenerator& ) = delete;
    ProtoFileGenerator& operator =(ProtoFileGenerator&& ) = delete;

public:
    void generate();

    const ProtoFile& getProtoFile() const
    {
        return m_protoFile;
    }

private:
    void generateMessages();
    void generateMessagesForFunction(llvm::Function* F);
    void generateMessageForType(llvm::Type* type);
    void generateMessageFields(llvm::Type* type, ProtoMessage& msg);
    ProtoMessage::Field generateMessageField(llvm::Type* type, const std::string& name, int fieldNum);

private:
    const Partition& m_partition;
    const std::string& m_protoName;
    ProtoFile m_protoFile;
    std::unordered_map<llvm::Type*, ProtoMessage> m_typeMessages;
}; // class ProtoFileGenerator

} // namespace vazgen

