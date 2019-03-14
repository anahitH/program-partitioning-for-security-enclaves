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
    void writeNamespace();
    void writeClasses();
    void writeFunctions();
    void writeGlobals();
    void writeClassDeclarations();
    void writeClassDefinitions();
    void writeClassDeclarations(const Class& class_);
    void writeClassDefinitions(const Class& class_);
    void writeClassDeclaration(const Class& class_);
    void writeClassDefinition(const Class& class_);
    void writeClassFunctionsDeclarations(const Class& class_);
    void writeClassMembers(const Class& class_);
    void writeFunctionDeclarations();
    void writeFunctionDefinitions();
    void writeFunctionDeclaration(const Function& F);
    void writeFunctionDefinition(const Function& F);

private:
    const SourceFile& m_sourceFile;
    FileWriter m_writer;
    int m_indent;
}; // class SourceFileWriter

} // namespace vazgen

