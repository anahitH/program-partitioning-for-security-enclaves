#include "ClangTools/ClangToolUtils.h"
#include "clang/AST/Decl.h"
#include "clang/AST/DeclCXX.h"

#include "clang/AST/Type.h"

namespace vazgen {

std::string ClangToolUtils::getScalarTypeName(const clang::Type* type)
{
    if (auto* typedefType = llvm::dyn_cast<clang::TypedefType>(type)) {
        auto* decl = typedefType->getDecl();
        return getScalarTypeName(&*decl->getUnderlyingType());
    }

    // TODO: test
    return type->getCanonicalTypeInternal().getAsString();
}

std::string ClangToolUtils::getTypeName(const clang::Type* type)
{
    if (type->isScalarType()) {
        return getScalarTypeName(type);
    }
    if (auto* typedefType = llvm::dyn_cast<clang::TypedefType>(type)) {
        return type->getCanonicalTypeInternal().getAsString();
    }
    if (auto* recordType = type->getAsStructureType()) {
        return recordType->getDecl()->getName();
    }
    return type->getCanonicalTypeInternal().getAsString();
}

} // namespace vazgen

