#include "CodeGen/SourceFileWriter.h"

#include <cassert>
#include <sstream>

namespace vazgen {

SourceFileWriter::SourceFileWriter(const std::string& name,
                                   const SourceFile& sourceFile)
    : m_writer(name)
    , m_sourceFile(sourceFile)
{
}

void SourceFileWriter::write()
{
    beginFile();
    writeNamespace();
}

void SourceFileWriter::beginFile()
{
    writeMacros();
    writeIncludes();
}

void SourceFileWriter::beginScope()
{
    ++m_indent;
    m_writer.write("{");
}

void SourceFileWriter::endScope()
{
    --m_indent;
    m_writer.write("}");
}

void SourceFileWriter::writeMacros()
{
    for (const auto& macro : m_sourceFile.getMacros()) {
        m_writer.write(macro);
    }
}

void SourceFileWriter::writeIncludes()
{
    for (const auto& include : m_sourceFile.getIncludes()) {
        m_writer.write("#include " + include);
    }
}

void SourceFileWriter::writeNamespace()
{
    const auto& namespace_ = m_sourceFile.getNamespace();
    if (!namespace_.empty()) {
        m_writer.write("namespace " + namespace_);
        beginScope();
    }
    writeClasses();
    if (!namespace_.empty()) {
        endScope();
    }
}

void SourceFileWriter::writeClasses()
{
    if (m_sourceFile.isHeader()) {
        writeClassDeclarations();
    } else {
        writeClassDefinitions();
    }
}

void SourceFileWriter::writeClassDeclarations()
{
    for (const auto& class_ : m_sourceFile.getClasses()) {
        writeClassDeclaration(class_);
    }
}

void SourceFileWriter::writeClassDefinitions()
{
    for (const auto& class_ : m_sourceFile.getClasses()) {
        writeClassDefinition(class_);
    }
}

void SourceFileWriter::writeClassDeclaration(const Class& class_)
{
    m_writer.write(class_.getClassDeclarationAsString() + " {");
    beginScope();
    writeClassFunctionsDeclarations(class_);
    writeClassMembers(class_);
    endScope();
    m_writer.write("};");
}

void SourceFileWriter::writeClassDefinition(const Class& class_)
{
    for (const auto& [access, function] : class_.getMemberFunctions()) {
        m_writer.write(function.getDefinitionAsString(class_.getName()));
    }
}

void SourceFileWriter::writeClassFunctionsDeclarations(const Class& class_)
{
    std::stringstream publicFStr;
    std::stringstream privateFStr;
    std::stringstream protectedFStr;
    for (const auto& [access, F] : class_.getMemberFunctions()) {
        if (access == Class::PUBLIC) {
            publicFStr << F.getDeclarationAsString();
        } else if (access == Class::PRIVATE) {
            privateFStr << F.getDeclarationAsString();
        } else if (access == Class::PROTECTED) {
            protectedFStr << F.getDeclarationAsString();
        } else {
            assert(false);
        }
    }
    m_writer.write("public:");
    beginScope();
    m_writer.write(publicFStr.str());
    endScope();
    m_writer.write("protected:");
    beginScope();
    m_writer.write(protectedFStr.str());
    endScope();
    m_writer.write("private:");
    beginScope();
    m_writer.write(privateFStr.str());
    endScope();
}

void SourceFileWriter::writeClassMembers(const Class& class_)
{
    std::stringstream publicFStr;
    std::stringstream privateFStr;
    std::stringstream protectedFStr;
    for (const auto& [access, mem] : class_.getMembers()) {
        if (access == Class::PUBLIC) {
            publicFStr << mem.getAsString();
        } else if (access == Class::PRIVATE) {
            privateFStr << mem.getAsString();
        } else if (access == Class::PROTECTED) {
            protectedFStr << mem.getAsString();
        } else {
            assert(false);
        }
    }
    m_writer.write("public:");
    beginScope();
    m_writer.write(publicFStr.str());
    endScope();
    m_writer.write("protected:");
    beginScope();
    m_writer.write(protectedFStr.str());
    endScope();
    m_writer.write("private:");
    beginScope();
    m_writer.write(privateFStr.str());
    endScope();
}


} // namespace vazgen

