#pragma once

#include "CodeGen/SGXCodeGenerator.h"
#include "CodeGen/SourceFile.h"

namespace vazgen {

class OpenEnclaveCodeGenerator : public SGXCodeGenerator
{
// TODO: consider having all qualifiers in one enum
public:
    static const std::string IN_PARAM;
    static const std::string OUT_PARAM;
    static const std::string IN_OUT_PARAM;

public:
    OpenEnclaveCodeGenerator(const std::string& programName,
                             const Functions& secureFunctions,
                             const Functions& appFunctions);

    OpenEnclaveCodeGenerator(const OpenEnclaveCodeGenerator&) = delete;
    OpenEnclaveCodeGenerator(OpenEnclaveCodeGenerator&&) = delete;
    OpenEnclaveCodeGenerator& operator =(const OpenEnclaveCodeGenerator&) = delete;
    OpenEnclaveCodeGenerator& operator =(OpenEnclaveCodeGenerator&&) = delete;

    virtual void generate() final;

private:
    virtual void writeGeneratedFiles() override;

    void generateEnclaveDefinitionFile();
    void generateEnclaveFile();
    void generateAppDriverFile();

private:
    SourceFile m_edlFile;
    SourceFile m_enclaveFile;
    SourceFile m_appDriverFile;
}; // class OpenEnclaveCodeGenerator

}

