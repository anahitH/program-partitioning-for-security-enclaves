#include "ClangTools/ProtoFileGenerator.h"

#include "clang/AST/Type.h"
#include "clang/AST/Decl.h"
#include "clang/AST/DeclCXX.h"

namespace vazgen {

namespace {

std::string getEnumType(const clang::EnumType* type)
{
    auto* enumDecl = llvm::dyn_cast<clang::EnumDecl>(type->getDecl());
    return enumDecl->getName();
}

std::string getScalarType(const clang::Type* type)
{
    if (type->isSignedIntegerType()) {
        return "int";
    }
    if (type->isUnsignedIntegerType()) {
        return "unsigned";
    }
    if (type->isFloatingType()) {
        return "float";
    }
}

std::string getMessageNameForType(const clang::Type* type)
{
    return type->getCanonicalTypeInternal().getAsString();
}

}

ProtoFileGenerator::ProtoFileGenerator(const Functions& functions, const std::string& protoName)
    : m_functions(functions)
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
        ProtoMessage msg(F->getName());
        for (unsigned i = 0; i < F->getNumParams(); ++i) {
            auto* paramDecl = F->getParamDecl(i);
            msg.addField(generateMessageField(&*paramDecl->getType(), paramDecl->getName(), (i+1)));
        }
        m_functionInputMessages.insert(std::make_pair(F, msg));
        m_protoFile.addMessage(msg);
    }
}

void ProtoFileGenerator::generateService()
{
    ProtoService service(m_protoName);
    for (auto* F : m_functions) {
        service.addRPC(generateRPC(F));
    }
    m_protoFile.addService(service);
}

void ProtoFileGenerator::generateMessagesForFunction(clang::FunctionDecl* F)
{
    generateMessageForType(&*F->getReturnType());
    for (unsigned i = 0; i < F->getNumParams(); ++i) {
        clang::QualType t = F->getParamDecl(i)->getType();
        llvm::dbgs() << t.getCanonicalType().getAsString() << "\n";
        generateMessageForType(&*F->getParamDecl(i)->getType());
    }
}

void ProtoFileGenerator::generateMessageForType(const clang::Type* type)
{
    if (m_typeMessages.find(type) != m_typeMessages.end()) {
        return;
    }
    if (auto* ptrType = llvm::dyn_cast<clang::PointerType>(type)) {
        generateMessageForType(&*ptrType->getPointeeType());
    }
    // No need to have message for builtin types
    if (type->isScalarType() || type->isArrayType()) {
        return;
    }
    ProtoMessage msg;
    msg.setName(getMessageNameForType(type));
    generateMessageFields(type, msg);
    m_typeMessages.insert(std::make_pair(type, msg));
    m_protoFile.addMessage(msg);
}

void ProtoFileGenerator::generateMessageFields(const clang::Type* type, ProtoMessage& msg)
{
    llvm::dbgs() << "Create fields for " << msg.getName() << "\n";
    clang::RecordDecl* recordDecl;
    if (auto* elaboratedType = llvm::dyn_cast<clang::ElaboratedType>(type)) {
        elaboratedType->getNamedType()->dump();
        elaboratedType->desugar()->dump();
        llvm::dbgs() << elaboratedType->getAsTagDecl()->getName() << "\n";
        generateMessageFields(&*elaboratedType->getNamedType(), msg);
    }
    if (auto* recordType = llvm::dyn_cast<clang::RecordType>(type)) {
        recordDecl = recordType->getDecl()->getDefinition();
    } else if (type->isStructureType()) {
        recordDecl = type->getAsStructureType()->getDecl();
    } else {
        return;
    }
    if (recordDecl->field_empty()) {
        llvm::dbgs() << "bo\n";
    }
    int i  = 0;
    for (auto it = recordDecl->field_begin(); it != recordDecl->field_end(); ++it) {
        const std::string& fieldName = it->getName().str();
        if (it->getType()->isEnumeralType()) {
            msg.addEnum(generateMessageEnum(&*it->getType()));
        }
        msg.addField(generateMessageField(&*it->getType(), fieldName, ++i));
    }
}

ProtoMessage::Enum ProtoFileGenerator::generateMessageEnum(const clang::Type* type)
{
    auto* enumType = llvm::dyn_cast<clang::EnumType>(type);
    auto* enumDecl = llvm::dyn_cast<clang::EnumDecl>(enumType->getDecl());
    ProtoMessage::Enum msgEnum;
    msgEnum.m_name = enumDecl->getName();
    for (auto it = enumDecl->enumerator_begin(); it != enumDecl->enumerator_end(); ++it) {
        if (it->getInitExpr()) {
            msgEnum.m_values.push_back(std::make_pair(it->getName(), it->getInitVal().getExtValue()));
        } else {
            msgEnum.m_values.push_back(std::make_pair(it->getName(), -1));
        }
    }
    return msgEnum;
}

ProtoMessage::Field ProtoFileGenerator::generateMessageField(const clang::Type* type, const std::string& name, int fieldNum)
{
    if (auto* ptrType = llvm::dyn_cast<clang::PointerType>(type)) {
        return generateMessageField(&*ptrType->getPointeeType(), name, fieldNum);
    }
    ProtoMessage::Field field;
    field.m_number = fieldNum;
    field.m_name = name;
    if (type->isScalarType()) {
        field.m_type = getScalarType(type);
        return field;
    }
    if (type->isEnumeralType()) {
        field.m_attribute = "FILL ENUM";
        field.m_type = getEnumType(llvm::dyn_cast<clang::EnumType>(type));
        return field;
    }
    if (auto* arrayType = llvm::dyn_cast<clang::ArrayType>(type)) {
        field = generateMessageField(&*arrayType->getElementType(), name, fieldNum);
        field.m_attribute = "repeated";
        return field;
    }
    // Composite types left
    auto pos = m_typeMessages.find(type);
    if (pos == m_typeMessages.find(type)) {
        generateMessageForType(type);
    }
    pos = m_typeMessages.find(type);
    field.m_type = pos->second.getName();
    return field;
}

ProtoService::RPC ProtoFileGenerator::generateRPC(const clang::FunctionDecl* F)
{
    ProtoService::RPC rpc;
    rpc.m_name = F->getName();
    rpc.m_input = m_functionInputMessages.find(F)->second;
    auto* returnType = &*F->getReturnType();
    if (auto* arrayType = llvm::dyn_cast<clang::ArrayType>(returnType)) {
        returnType = &*arrayType->getElementType();
    }
    rpc.m_output = m_typeMessages.find(returnType)->second;
    return rpc;
}

} // namespace vazgen

