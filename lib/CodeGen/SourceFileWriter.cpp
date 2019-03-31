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
    writeSourceScope(m_sourceFile);
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

void SourceFileWriter::writeSourceScope(const SourceScope& scope)
{
    writeClasses(scope.getClasses());
    writeFunctions(scope.getFunctions());
    writeGlobals(scope.getGlobalVariables());

    for (const auto& subScope : scope.getSubScopes()) {
        m_writer.write("namespace " + subScope->getName() + " {\n");
        writeSourceScope(*subScope);
        m_writer.write("} // namespace " + subScope->getName());
    }
}

void SourceFileWriter::writeClasses(const std::vector<Class>& classes)
{
    std::cout << "writing classes\n";
    if (m_sourceFile.isHeader()) {
        writeClassDeclarations(classes);
    } else {
        writeClassDefinitions(classes);
    }
}

void SourceFileWriter::writeFunctions(const std::vector<Function>& functions)
{
    std::cout << "Writing functions\n";
    if (m_sourceFile.isHeader()) {
        writeFunctionDeclarations(functions);
    } else {
        writeFunctionDefinitions(functions);
    }
}

void SourceFileWriter::writeGlobals(const std::vector<SourceScope::VariableValue>& globals)
{
    std::cout << "Writing globals\n";
    for (const auto& [global, value] : globals) {
        std::stringstream globalStrm;
        globalStrm << "static "
                   << global.getAsString();
        if (!value.empty()) {
            globalStrm << " = " << value;
        }
	globalStrm << ";";
        m_writer.write(globalStrm.str(), m_indent);
    }
}

void SourceFileWriter::writeClassDeclarations(const std::vector<Class>& classes)
{
    for (const auto& class_ : classes) {
        writeClassDeclaration(class_);
    }
}

void SourceFileWriter::writeClassDefinitions(const std::vector<Class>& classes)
{
    for (const auto& class_ : classes) {
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

void SourceFileWriter::writeFunctionDeclarations(const std::vector<Function>& functions)
{
    for (const auto& F : functions) {
        writeFunctionDeclaration(F);
    }
}

void SourceFileWriter::writeFunctionDefinitions(const std::vector<Function>& functions)
{
    for (const auto& F : functions) {
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

