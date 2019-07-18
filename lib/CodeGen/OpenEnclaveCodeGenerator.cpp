#include "CodeGen/OpenEnclaveCodeGenerator.h"
#include "CodeGen/SourceFileWriter.h"

#include <sstream>

namespace vazgen {

namespace {
Function generateFunctionForEdl(const Function& function, bool is_protected)
{
    std::string name = function.getName();
    if (is_protected) {
        name += "_protected";
    } else {
        name += "_app";
    }
    Function enclaveF(name);
    enclaveF.setReturnType(function.getReturnType());
    // TODO: think about having enum for access modifiers
    enclaveF.setAccessModifier("public");
    for (const auto& param : function.getParams()) {
        std::string qualifier = OpenEnclaveCodeGenerator::IN_PARAM;
        if (param.m_type.m_isPtr || param.m_type.m_isArray) {
            if (param.m_type.m_qualifier != "const") {
                qualifier = OpenEnclaveCodeGenerator::IN_OUT_PARAM;
            }
        }
        Type enclaveParamTy{param.m_type.m_name, qualifier, param.m_type.m_isPtr, param.m_type.m_isArray};
        enclaveF.addParam({enclaveParamTy, param.m_name});
    }
    return enclaveF;
}

Function generateMain()
{
    Function main("main");
    main.addParam(Variable{Type{"int", "", false, false}, "argc"});
    main.addParam(Variable{Type{"char", "const", true, true}, "argv"});
    main.setReturnType(Type{"int", "", false, false});
    main.addBody("create_enclave(argc, argv)");
    main.addBody("app_main()");
    main.addBody("return 0");
    return main;
}

std::string generateEcall(const Function& ecallF, const Variable returnVal)
{
    std::stringstream ecallStr;
    ecallStr << "oe_result_t oe_result = " << ecallF.getName();
    ecallStr << "(enclave, ";
    if (!ecallF.isVoidReturn()) {
        if (!ecallF.getReturnType().m_isPtr) {
            ecallStr << "&";
        }
        ecallStr << returnVal.m_name << ", ";
    }
    for (const auto& param : ecallF.getParams()) {
        ecallStr << param.m_name;
        if (param.m_name != ecallF.getParams().back().m_name) {
            ecallStr << ", ";
        }
    }
    ecallStr << ");";
    return ecallStr.str();
}

Function generateEcallWrapper(const Function& ecallF)
{
    Function wrapperF(ecallF.getName());
    wrapperF.setReturnType(ecallF.getReturnType());
    wrapperF.setParams(ecallF.getParams());

    std::stringstream wrapperBody;
    Variable returnVal;
    if (!ecallF.isVoidReturn()) {
        returnVal.m_type = ecallF.getReturnType();
        returnVal.m_name = "return_val";
        wrapperBody << returnVal.getAsString() << ";\n";
    }
    // TODO: handle pointer type for return value
    wrapperBody << generateEcall(ecallF, returnVal) << "\n";
    wrapperBody << "if (oe_result != OE_OK) {\n";
    wrapperBody << "fprintf(stderr, \"Failed to invoke ecall " << ecallF.getName() << "\");\n";
    wrapperBody << "terminate_enclave();\n";
    wrapperBody << "exit(1);\n";
    wrapperBody << "}\n";
    if (!ecallF.isVoidReturn()) {
        wrapperBody << "return " << returnVal.m_name;
    }
    wrapperF.addBody(wrapperBody.str());
    return wrapperF;
}

} // unnamed namespace

const std::string OpenEnclaveCodeGenerator::IN_PARAM = "[in]";
const std::string OpenEnclaveCodeGenerator::OUT_PARAM = "[out]";
const std::string OpenEnclaveCodeGenerator::IN_OUT_PARAM = "[in, out]";

OpenEnclaveCodeGenerator::OpenEnclaveCodeGenerator(const std::string& programName,
                                                   const Functions& secureFunctions,
                                                   const Functions& appFunctions)
    : SGXCodeGenerator(programName, secureFunctions, appFunctions)
{
}

void OpenEnclaveCodeGenerator::generate()
{
    // generate edl file
    generateEnclaveDefinitionFile();

    // generate enclave wrapper
    generateEnclaveFile();

    // generate app wrapper
    generateAppDriverFile();

    writeGeneratedFiles();
}

void OpenEnclaveCodeGenerator::writeGeneratedFiles()
{
    SourceFileWriter edlFileWriter(m_edlFile);
    edlFileWriter.write();

    //SourceFileWriter enclaveFileWriter(m_enclaveFile);
    //enclaveFileWriter.write();

    SourceFileWriter appDriverFileWriter(m_appDriverFile);
    appDriverFileWriter.write();
}

void OpenEnclaveCodeGenerator::generateEnclaveDefinitionFile()
{
    m_edlFile.setName("sgx.edl");
    m_edlFile.setHeader(true);

    SourceScope::ScopeType enclaceScope(new SourceScope("enclave"));
    SourceScope::ScopeType trustedScope(new SourceScope("trusted"));
    SourceScope::ScopeType untrustedScope(new SourceScope("untrusted"));

    for (const auto& secureF : m_enclaveFunctions) {
        trustedScope->addFunction(generateFunctionForEdl(secureF, true));
    }
    for (const auto& appF : m_appFunctions) {
        untrustedScope->addFunction(generateFunctionForEdl(appF, false));
    }
    enclaceScope->addSubscope(trustedScope);
    enclaceScope->addSubscope(untrustedScope);
    m_edlFile.addSubscope(enclaceScope);
}

void OpenEnclaveCodeGenerator::generateEnclaveFile()
{
    m_enclaveFile.setName(m_prefix + "_enclave.cc");
    m_enclaveFile.setHeader(false);
    m_enclaveFile.addInclude("<openenclave/enclave.h>");
    m_enclaveFile.addInclude("\"sgx_t.h\"");
}

void OpenEnclaveCodeGenerator::generateAppDriverFile()
{
    m_appDriverFile.setName(m_prefix + "_app.cc");
    m_appDriverFile.setHeader(false);
    m_appDriverFile.addInclude("\"sgx_u.h\"");
    // app_util.h has enclave creation and termination code
    m_appDriverFile.addInclude("\"app_utils.h\"");

    m_enclaveFile.addGlobalVariable(std::make_pair(Variable{Type{"oe_enclave_t", "", true, false}, "enclave"}, "NULL"));

    Function appMainF("app_main");
    appMainF.setIsExtern(true);
    appMainF.setReturnType(Type{"int", "", false, false});
    m_appDriverFile.addFunction(appMainF);


    m_appDriverFile.addFunction(generateMain());
    for (const auto& secureF : m_enclaveFunctions) {
        m_appDriverFile.addFunction(generateEcallWrapper(secureF));
    }
}

}

