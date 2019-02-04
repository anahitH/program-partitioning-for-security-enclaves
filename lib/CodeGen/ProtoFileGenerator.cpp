#include "CodeGen/ProtoFileGenerator.h"
#include "Analysis/Partition.h"

#include "llvm/IR/Function.h"
#include "llvm/IR/Type.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Constants.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/raw_ostream.h"

namespace vazgen {

namespace {

std::string getFieldName(llvm::Type* type)
{
    if (auto* structType = llvm::dyn_cast<llvm::StructType>(type)) {
        return structType->getName().str() + "Value";
    }
    if (auto* array = llvm::dyn_cast<llvm::ArrayType>(type)) {
        return "arrayValue";
    }
    return "value";
}

std::string getMessageNameForScalar(llvm::Type* type)
{
    if (auto* intType = llvm::dyn_cast<llvm::IntegerType>(type)) {
        switch(intType->getBitWidth())
        {
        case 1:
            return "Bool";
        case 32:
            return "Integer32";
        case 64:
            return "Integer64";
        default:
            return "Integer";
        };
    }
    if (type->isFloatTy()) {
        return "Float";
    }
    if (type->isDoubleTy()) {
        return "Double";
    }
    assert(false);
    return "";
}

std::string getScalarType(llvm::Type* type)
{
    if (auto* intType = llvm::dyn_cast<llvm::IntegerType>(type)) {
        switch(intType->getBitWidth())
        {
        case 1:
            return "bool";
        case 32:
            return "int32";
        case 64:
            return "int64";
        default:
            return "int";
        };
    }
    if (type->isFloatTy()) {
        return "float";
    }
    if (type->isDoubleTy()) {
        return "double";
    }
    assert(false);
    return "";
}

std::string getMessageNameForType(llvm::Type* type)
{
    if (!llvm::dyn_cast<llvm::CompositeType>(type)) {
        return getMessageNameForScalar(type);
    }
    if (auto* structType = llvm::dyn_cast<llvm::StructType>(type)) {
        return structType->getName();
    }
    if (auto* arrayType = llvm::dyn_cast<llvm::ArrayType>(type)) {
        return getMessageNameForType(arrayType->getElementType()) + "Array";
    }
    // TODO: more types?
    assert(false);
    return "";
}

} // unnamed namespace


ProtoFileGenerator::ProtoFileGenerator(const Partition& partition, const std::string& protoName)
    : m_partition(partition)
    , m_protoName(protoName)
{
}

void ProtoFileGenerator::generate()
{
    m_protoFile.setVersion("proto3");
    m_protoFile.setPackage(m_protoName);

    generateMessages();
}

void ProtoFileGenerator::generateMessages()
{
    for (auto* F : m_partition.getPartition()) {
        generateMessagesForFunction(F);
    }
}

void ProtoFileGenerator::generateMessagesForFunction(llvm::Function* F)
{
    generateMessageForType(F->getReturnType());
    auto* Ftype = F->getFunctionType();
    for (unsigned i = 0; i < Ftype->getNumParams(); ++i) {
        generateMessageForType(Ftype->getParamType(i));
    }
}

void ProtoFileGenerator::generateMessageForType(llvm::Type* type)
{
    if (m_typeMessages.find(type) != m_typeMessages.end()) {
        return;
    }
    if (auto* ptrType = llvm::dyn_cast<llvm::PointerType>(type)) {
        generateMessageForType(ptrType->getElementType());
    }
    ProtoMessage msg;
    msg.setName(getMessageNameForType(type));
    generateMessageFields(type, msg);
    m_typeMessages.insert(std::make_pair(type, msg));
    m_protoFile.addMessage(msg);
}

void ProtoFileGenerator::generateMessageFields(llvm::Type* type, ProtoMessage& msg)
{
    if (auto* structType = llvm::dyn_cast<llvm::StructType>(type)) {
        for (int i = 0; i < structType->getNumElements(); ++i) {
            msg.addField(generateMessageField(structType->getElementType(i),
                         getFieldName(structType->getElementType(i)),
                         (i+1)));
        }
    } else {
        msg.addField(generateMessageField(type, "value", 1));
    }
}

ProtoMessage::Field ProtoFileGenerator::generateMessageField(llvm::Type* type, const std::string& name, int fieldNum)
{
    if (auto* ptrType = llvm::dyn_cast<llvm::PointerType>(type)) {
        return generateMessageField(ptrType->getElementType(), name, fieldNum);
    }
    ProtoMessage::Field field;
    field.m_number = fieldNum;
    field.m_name = name;
    if (!llvm::isa<llvm::CompositeType>(type)) {
        field.m_type = getScalarType(type);
        return field;
    }
    if (auto* arrayType = llvm::dyn_cast<llvm::ArrayType>(type)) {
        field = generateMessageField(arrayType->getElementType(), name, fieldNum);
        field.m_attribute = "repeated";
        return field;
    }
    assert(llvm::isa<llvm::CompositeType>(type));
    auto pos = m_typeMessages.find(type);
    if (pos == m_typeMessages.end()) {
        generateMessageForType(type);
    }
    pos = m_typeMessages.find(type);
    field.m_type = pos->second.getName();
    return field;
}

} // namespace vazgen

