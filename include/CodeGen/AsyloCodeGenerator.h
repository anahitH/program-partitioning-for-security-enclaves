#pragma once

#include "CodeGen/SGXCodeGenerator.h"
#include "CodeGen/SourceFile.h"

#include <vector>

namespace vazgen {

class AsyloCodeGenerator : public SGXCodeGenerator
{
public:
    struct FunctionSelector
    {
        std::string m_functionName;
        std::string m_selectorName;
        std::string m_selectorValue;
    };

public:
    AsyloCodeGenerator(const std::string& programName,
                     const Functions& secureFunctions,
                     const Functions& appFunctions);

    AsyloCodeGenerator(const AsyloCodeGenerator&) = delete;
    AsyloCodeGenerator(AsyloCodeGenerator&&) = delete;
    AsyloCodeGenerator& operator =(const AsyloCodeGenerator&) = delete;
    AsyloCodeGenerator& operator =(AsyloCodeGenerator&&) = delete;

    virtual void generate() final;

private:
    void generateInterfaceSelectors();
    void generateEnclaveRunner();
    void generateAppDriver();
    void generateEnclaveAbortFunction(SourceScope::ScopeType& inScope);
    void generateEnclaveEcalls(SourceScope::ScopeType& inScope);
    std::vector<Function> generateAppFunctionsInEnclave();
    void generateEnclaveEcall(const Function& enclaveF, SourceScope::ScopeType& inScope);
    Function generateAppFunctionWrapperInEnclave(const Function appF);
    void generateAppFunctionInEnclave(const Function& ocallWrapper, const Function& appF);
    void generateAsyloEnclaveInitFunction(SourceScope::ScopeType& inScope);
    void generateAsyloEnclaveFiniFunction(SourceScope::ScopeType& inScope);
    void generateAppDriverMain();
    void generateOCallRegistration(SourceScope::ScopeType& inScope);
    void generateOCalls(SourceScope::ScopeType inScope);
    void generateOCall(const Function& appF, SourceScope::ScopeType inScope);
    std::vector<Function> generateEnclaveFunctionsInDriver();
    Function generateEcallWrapper(const Function& enclaveF);
    void generateEnclaveFunctionInDriver(const Function& ecallWrapper, const Function& enclaveF);

    void writeGeneratedFiles();

private:
    std::unordered_map<std::string, FunctionSelector> m_ecallSelectors;
    std::unordered_map<std::string, FunctionSelector> m_ocallSelectors;
    SourceFile m_interfaceSelectorsFile;
    SourceFile m_enclaveFile;
    SourceFile m_appDriverFile;
}; // class AsyloCodeGenerator

} // namespace vazgen

