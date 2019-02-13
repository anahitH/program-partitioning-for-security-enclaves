#pragma once

#include "CodeGen/ProtoFile.h"
#include <unordered_set>
#include <unordered_map>

namespace clang {
class FunctionDecl;
class Type;
class QualType;
class FieldDecl;
class NamedDecl;
class ValueDecl;
}

namespace vazgen {

class ProtoFileGenerator
{
public:
    struct Struct
    {
        struct Field {
            const clang::Type* m_type;
            std::string m_name;
        };
        using Fields = std::vector<Field>;
        std::string m_name;
        Fields m_fields;
    };
    struct Enum
    {
        struct Entry {
            std::string m_name;
            long m_value;
        };
        using Entries = std::vector<Entry>;
        std::string m_name;
        Entries m_entries;
    };
    using Enums = std::unordered_map<std::string, Enum>;
    using Structs = std::unordered_map<std::string, Struct>;
    using Functions = std::unordered_set<const clang::FunctionDecl*>;

public:
    ProtoFileGenerator() = default;
    ProtoFileGenerator(const Functions& function,
                       const Structs& structs,
                       const Enums& enums,
                       const std::string& protoName);
    
    ProtoFileGenerator(const ProtoFileGenerator& ) = delete;
    ProtoFileGenerator(ProtoFileGenerator&& ) = delete;
    ProtoFileGenerator& operator =(const ProtoFileGenerator& ) = delete;
    ProtoFileGenerator& operator =(ProtoFileGenerator&& ) = delete;

public:
    void setFunctions(const Functions& functions)
    {
        m_functions = functions;
    }

    void setStructs(const Structs& structs)
    {
        m_structs = structs;
    }

    void setEnums(const Enums& enums)
    {
        m_enums = enums;
    }

    void setProtoName(const std::string& protoName)
    {
        m_protoName = protoName;
    }

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
    void generateMessageForArrayType(const clang::Type* elementType);
    void generateMessageFields(const clang::Type* type, ProtoMessage& msg);
    ProtoMessage::Enum generateMessageEnum(const clang::Type* type);
    ProtoMessage::Field generateMessageField(const clang::Type* type, const std::string& name, int fieldNum);
    ProtoService::RPC generateRPC(const clang::FunctionDecl* F);

private:
    Functions m_functions;
    Structs m_structs;
    Enums m_enums;
    std::string m_protoName;
    ProtoFile m_protoFile;
    std::unordered_map<std::string, ProtoMessage> m_typeMessages;
    std::unordered_map<const clang::FunctionDecl*, ProtoMessage> m_functionInputMessages;
}; // class ProtoFileGenerator

}

