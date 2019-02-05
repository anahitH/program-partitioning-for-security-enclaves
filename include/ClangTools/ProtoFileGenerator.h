#pragma once

#include "CodeGen/ProtoFile.h"
#include <unordered_set>
#include <unordered_map>

namespace clang {
class FunctionDecl;
class Type;
class QualType;
}

namespace vazgen {

class ProtoFileGenerator
{
public:
    using Functions = std::unordered_set<const clang::FunctionDecl*>;

public:
    ProtoFileGenerator(const Functions& function, const std::string& protoName);
    
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
    void generateRPCMessages();
    void generateService();
    void generateMessagesForFunction(clang::FunctionDecl* F);
    void generateMessageForType(const clang::Type* type);
    void generateMessageFields(const clang::Type* type, ProtoMessage& msg);
    ProtoMessage::Enum generateMessageEnum(const clang::Type* type);
    ProtoMessage::Field generateMessageField(const clang::Type* type, const std::string& name, int fieldNum);
    ProtoService::RPC generateRPC(const clang::FunctionDecl* F);

private:
    Functions m_functions;
    const std::string& m_protoName;
    ProtoFile m_protoFile;
    std::unordered_map<const clang::Type*, ProtoMessage> m_typeMessages;
    std::unordered_map<const clang::FunctionDecl*, ProtoMessage> m_functionInputMessages;
}; // class ProtoFileGenerator

}

