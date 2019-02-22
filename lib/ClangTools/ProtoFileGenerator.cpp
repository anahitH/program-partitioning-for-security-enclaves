#include "ClangTools/ProtoFileGenerator.h"

#include "clang/AST/Type.h"
#include "clang/AST/Decl.h"
#include "clang/AST/DeclCXX.h"

namespace vazgen {

namespace {

std::string getEnumType(const clang::EnumType* type)
{
    return  type->getCanonicalTypeInternal().getAsString();
}

std::string getScalarType(const clang::Type* type)
{
    // The order is important
    if (type->isCharType()) {
        return "int32";
    }
    if (auto* builtin = llvm::dyn_cast<clang::BuiltinType>(type)) {
        if (builtin->getKind() == clang::BuiltinType::Bool) {
            return "bool";
        }
        if (builtin->getKind() == clang::BuiltinType::Float) {
            return "float";
        }
        if (builtin->getKind() == clang::BuiltinType::Double) {
            return "double";
        }
        if (builtin->getKind() == clang::BuiltinType::UInt) {
            return "uint32";
        }
        if (builtin->getKind() == clang::BuiltinType::Int) {
            return "int32";
        }
        if (builtin->getKind() == clang::BuiltinType::Long) {
            return "int64";
        }
        if (builtin->getKind() == clang::BuiltinType::ULong) {
            return "uint64";
        }
        if (builtin->getKind() == clang::BuiltinType::Short) {
            return "int32";
        }
        if (builtin->getKind() == clang::BuiltinType::UShort) {
            return "uint32";
        }
    }
    if (auto* typedefType = llvm::dyn_cast<clang::TypedefType>(type)) {
        auto* decl = typedefType->getDecl();
        return getScalarType(&*decl->getUnderlyingType());
    }
    return "FILL";
}

std::string getScalarTypeForC(const clang::Type* type)
{
    if (auto* typedefType = llvm::dyn_cast<clang::TypedefType>(type)) {
        auto* decl = typedefType->getDecl();
        return getScalarTypeForC(&*decl->getUnderlyingType());
    }
    if (auto* builtin = llvm::dyn_cast<clang::BuiltinType>(type)) {
        if (builtin->getKind() == clang::BuiltinType::UInt
                || builtin->getKind() == clang::BuiltinType::Int
                || builtin->getKind() == clang::BuiltinType::Short
                || builtin->getKind() == clang::BuiltinType::UShort) {
            return getScalarType(type) + "_t";
        }
        if (builtin->getKind() == clang::BuiltinType::Long) {
            return "long";
        }
        if (builtin->getKind() == clang::BuiltinType::ULong) {
            return "unsigned long";
        }
    }
    return getScalarType(type);
}

std::string getMessageNameForType(const clang::Type* type)
{
    if (auto* typedefType = llvm::dyn_cast<clang::TypedefType>(type)) {
        return type->getCanonicalTypeInternal().getAsString();
    }
    if (auto* recordType = type->getAsStructureType()) {
        return recordType->getDecl()->getName();
    }
    return type->getCanonicalTypeInternal().getAsString();
}

std::string getMessageNameForArrayType(const clang::Type* type)
{
    if (auto* arrayType = llvm::dyn_cast<clang::ArrayType>(type)) {
        return getMessageNameForArrayType(&*arrayType->getElementType());
    }
    return getMessageNameForType(type)+"Array";
}

}

ProtoFileGenerator::ProtoFileGenerator(const Functions& functions,
                                       const Structs& structs,
                                       const Enums& enums,
                                       const std::string& protoName)
    : m_functions(functions)
    , m_structs(structs)
    , m_enums(enums)
    , m_protoName(protoName)
{
}

void ProtoFileGenerator::generate()
{
    m_protoFile.setVersion("proto3");
    m_protoFile.setPackage(m_protoName);

    generateMessages();
    generateRPCMessages();
    generateService();
}

void ProtoFileGenerator::generateMessages()
{
    for (auto* F : m_functions) {
        generateMessagesForFunction(const_cast<clang::FunctionDecl*>(F));
    }
}

void ProtoFileGenerator::generateRPCMessages()
{
    for (auto* F : m_functions) {
        //llvm::dbgs() << "Creating RPC message for function " << F->getName() << "\n";
        ProtoMessage msg(F->getName().str() + "_INPUT");
        for (unsigned i = 0; i < F->getNumParams(); ++i) {
            auto* paramDecl = F->getParamDecl(i);
            auto field = generateMessageField(&*paramDecl->getType(), paramDecl->getName(), (i+1));
            if (llvm::isa<clang::PointerType>(&*paramDecl->getType())) {
                field.m_isPtr = true;
            }
            msg.addField(field);
        }
        m_functionInputMessages.insert(std::make_pair(F, msg));
        m_protoFile.addMessage(msg);
    }
}

void ProtoFileGenerator::generateService()
{
    if (!m_protoFile.hasService(m_protoName)) {
        m_protoFile.addService(ProtoService(m_protoName));
    }
    ProtoService& service = m_protoFile.getService(m_protoName); 
    for (auto* F : m_functions) {
        service.addRPC(generateRPC(F));
    }
    //m_protoFile.addService(service);
}

void ProtoFileGenerator::generateMessagesForFunction(clang::FunctionDecl* F)
{
    //llvm::dbgs() << "Create Message for function " << F->getName() << "\n";
    auto* returnType = &*F->getReturnType();
    if (!returnType->isVoidType()) {
        generateMessageForType(returnType);
    }
    for (unsigned i = 0; i < F->getNumParams(); ++i) {
        clang::QualType t = F->getParamDecl(i)->getType();
        generateMessageForType(&*F->getParamDecl(i)->getType());
    }
}

void ProtoFileGenerator::generateMessageForType(const clang::Type* type)
{
    if (auto* ptrType = llvm::dyn_cast<clang::PointerType>(type)) {
        generateMessageForType(&*ptrType->getPointeeType());
    }
    // No need to have message for builtin types
    if (type->isScalarType() || type->isArrayType()) {
        return;
    }
    const auto& typeName = getMessageNameForType(type);
    if (m_typeMessages.find(typeName) != m_typeMessages.end()) {
        return;
    }
    ProtoMessage msg;
    msg.setName(typeName);
    generateMessageFields(type, msg);
    m_typeMessages.insert(std::make_pair(typeName, msg));
    m_protoFile.addMessage(msg);
}

void ProtoFileGenerator::generateMessageForArrayType(const clang::Type* elementType)
{
    if (auto* ptrType = llvm::dyn_cast<clang::PointerType>(elementType)) {
        elementType = &*ptrType->getPointeeType();
    }
    ProtoMessage msg;
    const std::string name = getMessageNameForArrayType(elementType);
    if (m_typeMessages.find(name) != m_typeMessages.end()) {
        return;
    }
    msg.setName(name);
    msg.addField(generateMessageField(elementType, "element", 1));
    m_typeMessages.insert(std::make_pair(name, msg));
    m_protoFile.addMessage(msg);
}

void ProtoFileGenerator::generateMessageFields(const clang::Type* type, ProtoMessage& msg)
{
    //llvm::dbgs() << "Create fields for " << msg.getName() << "\n";
    const auto& structFields = m_structs.find(msg.getName());
    if (structFields == m_structs.end()) {
        msg.addField(ProtoMessage::Field{"struct", "type not found", "fill manually"});
        return;
    }
    int i  = 0;
    for (const auto& field : structFields->second.m_fields) {
        const std::string& fieldName = field.m_name;
        if (field.m_type->isEnumeralType()) {
            msg.addEnum(generateMessageEnum(&*field.m_type));
        }
        msg.addField(generateMessageField(&*field.m_type, fieldName, ++i));
    }
}

ProtoMessage::Enum ProtoFileGenerator::generateMessageEnum(const clang::Type* type)
{
    ProtoMessage::Enum msgEnum;
    const std::string& enumName = getMessageNameForType(type);
    const auto& enum_pos = m_enums.find(enumName);
    if (enum_pos == m_enums.end()) {
        msgEnum.m_name = "FILL IN";
        return msgEnum;
    }
    msgEnum.m_name = enumName; //enumDecl->getName();
    const auto& enumEntries = enum_pos->second.m_entries;
    for (const auto& entry : enumEntries) {
        msgEnum.m_values.push_back(std::make_pair(entry.m_name, entry.m_value));
    }
    return msgEnum;
}

ProtoMessage::Field ProtoFileGenerator::generateMessageField(const clang::Type* type, const std::string& name, int fieldNum)
{
    if (auto* ptrType = llvm::dyn_cast<clang::PointerType>(type)) {
        return generateMessageField(&*ptrType->getPointeeType(), name, fieldNum);
    }
    if (auto* decayedType = llvm::dyn_cast<clang::DecayedType>(type)) {
        return generateMessageField(&*decayedType->getOriginalType(), name, fieldNum);
    }

    ProtoMessage::Field field;
    field.m_number = fieldNum;

    if (type->isEnumeralType()) {
        field.m_type = getMessageNameForType(type);
        field.m_Ctype = field.m_type;
        field.m_name = name == field.m_type ? name + "_var" : name;
        return field;
    }
    if (type->isScalarType()) {
        field.m_type = getScalarType(type);
        field.m_Ctype = getScalarTypeForC(type);
        field.m_name = name == field.m_type ? name + "_var" : name;
        return field;
    }
    if (auto* arrayType = llvm::dyn_cast<clang::ArrayType>(type)) {
        auto* elementType = &*arrayType->getElementType();
        if (elementType->isArrayType()) {
            generateMessageForArrayType(elementType);
            auto pos = m_typeMessages.find(getMessageNameForArrayType(elementType));
            field.m_attribute = "repeated";
            field.m_type = pos->second.getName();
            field.m_Ctype = "FILL";
            field.m_name = name == field.m_type ? name + "_var" : name;
            return field;
        }
        field = generateMessageField(elementType, name, fieldNum);
        field.m_Ctype = "FILL";
        field.m_attribute = "repeated";
        return field;
    }
    // Composite types left
    auto pos = m_typeMessages.find(getMessageNameForType(type));
    if (pos == m_typeMessages.end()) {
        generateMessageForType(type);
        pos = m_typeMessages.find(getMessageNameForType(type));
    }
    field.m_type = pos->second.getName();
    field.m_Ctype = field.m_type;
    field.m_name = name == field.m_type ? name + "_var" : name;
    return field;
}

ProtoService::RPC ProtoFileGenerator::generateRPC(const clang::FunctionDecl* F)
{
    ProtoService::RPC rpc;
    rpc.m_name = F->getName();
    rpc.m_input = m_functionInputMessages.find(F)->second;

    ProtoMessage outputMsg;
    outputMsg.setName(rpc.m_name + "_OUT");
    // In ideal case will add only fields wich where pointer types
    outputMsg.setFields(rpc.m_input.getFields());
    outputMsg.setEnums(rpc.m_input.getEnums());
    auto* returnType = &*F->getReturnType();
    if (!returnType->isVoidType()) {
        outputMsg.addField(generateMessageField(returnType, "returnVal", outputMsg.getFields().size()));
    }
    m_protoFile.addMessage(outputMsg);
    rpc.m_output = outputMsg;
    return rpc;
}

} // namespace vazgen

