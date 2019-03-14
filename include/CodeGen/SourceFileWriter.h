#pragma once

#include "CodeGen/FileWriter.h"
#include "CodeGen/SourceFile.h"

namespace vazgen {

class SourceFileWriter
{
public:
   SourceFileWriter(const SourceFile& sourceFile);

   SourceFileWriter(const SourceFileWriter& writer) = delete;
   SourceFileWriter(SourceFileWriter&& writer) = delete;
   SourceFileWriter& operator =(const SourceFileWriter& ) = delete;
   SourceFileWriter& operator =(SourceFileWriter&& ) = delete;

public:
    void write();

public:
    void beginFile();
    void beginScope();
    void endScope();
    void writeMacros();
    void writeIncludes();
    void writeSourceScope(const SourceScope& scope);
    void writeClasses(const std::vector<Class>& classes);
    void writeFunctions(const std::vector<Function>& functions);
    void writeGlobals(const std::vector<SourceScope::VariableValue>& globals);
    void writeClassDeclarations(const std::vector<Class>& classes);
    void writeClassDefinitions(const std::vector<Class>& classes);
    void writeClassDeclarations(const Class& class_);
    void writeClassDefinitions(const Class& class_);
    void writeClassDeclaration(const Class& class_);
    void writeClassDefinition(const Class& class_);
    void writeClassFunctionsDeclarations(const Class& class_);
    void writeClassMembers(const Class& class_);
    void writeFunctionDeclarations(const std::vector<Function>& functions);
    void writeFunctionDefinitions(const std::vector<Function>& functions);
    void writeFunctionDeclaration(const Function& F);
    void writeFunctionDefinition(const Function& F);

private:
    const SourceFile& m_sourceFile;
    FileWriter m_writer;
    int m_indent;
}; // class SourceFileWriter

} // namespace vazgen

