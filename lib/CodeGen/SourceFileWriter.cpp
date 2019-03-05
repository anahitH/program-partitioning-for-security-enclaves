#include "CodeGen/SourceFileWriter.h"

#include <cassert>
#include <sstream>
#include <iostream>

namespace vazgen {

SourceFileWriter::SourceFileWriter(const SourceFile& sourceFile)
    : m_writer(sourceFile.getName())
    , m_sourceFile(sourceFile)
    , m_indent(0)
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
}

void SourceFileWriter::endScope()
{
    --m_indent;
}

void SourceFileWriter::writeMacros()
{
    for (const auto& macro : m_sourceFile.getMacros()) {
        m_writer.write(macro);
    }
    if (!m_sourceFile.getMacros().empty()) {
        m_writer.writeNewLine();
    }
}

void SourceFileWriter::writeIncludes()
{
    for (const auto& include : m_sourceFile.getIncludes()) {
        m_writer.write("#include " + include);
    }
    m_writer.writeNewLine();
}

void SourceFileWriter::writeNamespace()
{
    const auto& namespace_ = m_sourceFile.getNamespace();
    if (!namespace_.empty()) {
        m_writer.write("namespace " + namespace_ + " {\n");
    }
    writeClasses();
    writeFunctions();
    if (!namespace_.empty()) {
        m_writer.write("} // namespace " + namespace_);
    }
}

void SourceFileWriter::writeClasses()
{
    std::cout << "writing classes\n";
    if (m_sourceFile.isHeader()) {
        writeClassDeclarations();
    } else {
        writeClassDefinitions();
    }
}

void SourceFileWriter::writeFunctions()
{
    std::cout << "Writing functions\n";
    if (m_sourceFile.isHeader()) {
        writeFunctionDeclarations();
    } else {
        writeFunctionDefinitions();
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
    std::cout << "writing class decl " << class_.getName() << "\n";
    m_writer.write(class_.getClassDeclarationAsString() + "{");
    beginScope();
    writeClassFunctionsDeclarations(class_);
    writeClassMembers(class_);
    endScope();
    m_writer.write("}; // class " + class_.getName());
}

void SourceFileWriter::writeClassDefinition(const Class& class_)
{
    for (const auto& [access, functions] : class_.getMemberFunctions()) {
        for (const auto& function : functions) {
            m_writer.write(function.getDefinitionAsString(class_.getName()) + "\n", m_indent);
        }
    }
}

void SourceFileWriter::writeClassFunctionsDeclarations(const Class& class_)
{
    bool pub = false;
    bool priv = false;
    bool prot = false;
    for (const auto& [access, functions] : class_.getMemberFunctions()) {
        for (const auto& F : functions) {
            if (access == Class::PUBLIC) {
                if (!pub) {
                    m_writer.write("public");
                    pub = true;
                }
            } else if (access == Class::PRIVATE) {
                if (!priv) {
                    m_writer.write("private:");
                    priv = true;
                }
            } else if (access == Class::PROTECTED) {
                if (!prot) {
                    m_writer.write("protected:");
                    prot = true;
                }
            } else {
                assert(false);
            }
            m_writer.write(F.getDeclarationAsString(), m_indent);
        }
    }
}

void SourceFileWriter::writeClassMembers(const Class& class_)
{
    std::stringstream publicFStr;
    std::stringstream privateFStr;
    std::stringstream protectedFStr;
    for (const auto& [access, mems] : class_.getMembers()) {
        for (const auto& mem : mems) {
            if (access == Class::PUBLIC) {
                publicFStr << mem.getAsString() << "\n";
            } else if (access == Class::PRIVATE) {
                privateFStr << mem.getAsString() << "\n";
            } else if (access == Class::PROTECTED) {
                protectedFStr << mem.getAsString() << "\n";
            } else {
                assert(false);
            }
        }
    }
    if (!publicFStr.str().empty()) {
        m_writer.write("public:");
        m_writer.write(publicFStr.str(), m_indent);
    }
    if (!protectedFStr.str().empty()) {
        m_writer.write("protected:");
        m_writer.write(protectedFStr.str(), m_indent);
    }
    if (!privateFStr.str().empty()) {
        m_writer.write("private:");
        m_writer.write(privateFStr.str(), m_indent);
    }
}

void SourceFileWriter::writeFunctionDeclarations()
{
    for (const auto& F : m_sourceFile.getFunctions()) {
        writeFunctionDeclaration(F);
    }
}

void SourceFileWriter::writeFunctionDefinitions()
{
    for (const auto& F : m_sourceFile.getFunctions()) {
        writeFunctionDefinition(F);
    }
}

void SourceFileWriter::writeFunctionDeclaration(const Function& F)
{
    m_writer.write(F.getDeclarationAsString(), m_indent);
}

void SourceFileWriter::writeFunctionDefinition(const Function& F)
{
    m_writer.write(F.getDefinitionAsString() + "\n", m_indent);
}

} // namespace vazgen

