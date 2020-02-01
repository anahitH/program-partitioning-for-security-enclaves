#include "CodeGen/OpenEnclaveCodeGenerator.h"
#include "CodeGen/OpenEnclaveSupportedLibFunctions.h"
#include "CodeGen/SourceFileWriter.h"

#include <sstream>

namespace vazgen {

namespace {

std::string getEdlParamQualifier(const Type paramty)
{
    if (!paramty.m_isPtr && !paramty.m_isArray) {
	    return "";
    }
    std::string qualifier = OpenEnclaveCodeGenerator::IN_PARAM;
    if (paramty.m_isPtr || paramty.m_isArray) {
	    if (paramty.m_qualifier != "const") {
		    qualifier = OpenEnclaveCodeGenerator::IN_OUT_PARAM;
	    }
    }
    return qualifier;
}

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
    if (is_protected) {
        enclaveF.setAccessModifier("public");
    }
    for (const auto& param : function.getParams()) {
	std::string qualifier = getEdlParamQualifier(param.m_type);
        Type enclaveParamTy{param.m_type.m_name, qualifier, param.m_type.m_isPtr || param.m_type.m_isArray, false};
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

std::string generateOcall(const Function& appF, const Variable returnVal)
{
    std::stringstream ocallStr;
    ocallStr << "oe_result_t oe_result = " << appF.getName() << "_app(";
    if (!appF.isVoidReturn()) {
        if (!appF.getReturnType().m_isPtr) {
            ocallStr << "&";
        }
        ocallStr << returnVal.m_name << ", ";
    }
    for (const auto& param : appF.getParams()) {
        ocallStr << param.m_name;
        if (param.m_name != appF.getParams().back().m_name) {
            ocallStr << ", ";
        }
    }
    ocallStr << ");";
    return ocallStr.str();
}

std::string generateEcall(const Function& ecallF, const Variable returnVal)
{
    std::stringstream ecallStr;
    ecallStr << "oe_result_t oe_result = " << ecallF.getName() << "_protected";
    ecallStr << "(enclave, ";
    if (!ecallF.isVoidReturn()) {
        if (!ecallF.getReturnType().m_isPtr) {
            ecallStr << "&";
        }
        ecallStr << returnVal.m_name << ", ";
    }
    for (const auto& param : ecallF.getParams()) {
        // TODO: check if need to handle array params
        ecallStr << param.m_name;
        if (param.m_name != ecallF.getParams().back().m_name) {
            ecallStr << ", ";
        }
    }
    ecallStr << ");";
    return ecallStr.str();
}

Function generateExternForFunction(const Function& enclaveF)
{
    Function externF = enclaveF;
    externF.setIsExtern(true);
    return externF;
}

Function generateCallWrapper(const Function& F, bool is_ecall)
{
    std::string name = F.getName();
    if (is_ecall) {
        name += "_protected";
    } else {
        name += "_app";
    }

    Function wrapperF(name);
    wrapperF.setReturnType(F.getReturnType());
    wrapperF.setParams(F.getParams());
    std::string Fcall = F.getCallAsString(F.getParams());
    std::string body;
    if (!F.isVoidReturn()) {
        body = "return " + Fcall;
    } else {
        body = Fcall;
    }
    wrapperF.addBody(body);
    return wrapperF;
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

Function generateOcallWrapper(const Function& appF)
{
    Function wrapperF(appF.getName());
    wrapperF.setReturnType(appF.getReturnType());
    wrapperF.setParams(appF.getParams());

    std::stringstream wrapperBody;
    Variable returnVal;
    if (!appF.isVoidReturn()) {
        returnVal.m_type = appF.getReturnType();
        returnVal.m_name = "return_val";
        wrapperBody << returnVal.getAsString() << ";\n";
    }
    // TODO: handle pointer type for return value
    wrapperBody << generateOcall(appF, returnVal) << "\n";
    wrapperBody << "if (oe_result != OE_OK) {\n";
    wrapperBody << "fprintf(stderr, \"Failed to invoke ocall " << appF.getName() << "\");\n";
    // TODO: see how to handle failure of ocalls Terminate enclave??
    //wrapperBody << "terminate_enclave();\n";
    //wrapperBody << "exit(1);\n";
    wrapperBody << "}\n";
    if (!appF.isVoidReturn()) {
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

    SourceFileWriter enclaveFileWriter(m_enclaveFile);
    enclaveFileWriter.write();

    SourceFileWriter appDriverFileWriter(m_appDriverFile);
    appDriverFileWriter.write();
}

void OpenEnclaveCodeGenerator::generateEnclaveDefinitionFile()
{
    m_edlFile.setName(m_prefix + ".edl");
    m_edlFile.setHeader(true);

    SourceScope::ScopeType enclaceScope(new SourceScope("enclave", false));
    SourceScope::ScopeType trustedScope(new SourceScope("trusted", false));
    SourceScope::ScopeType untrustedScope(new SourceScope("untrusted", false));

    for (const auto& secureF : m_enclaveFunctions) {
        trustedScope->addFunction(generateFunctionForEdl(secureF, true));
    }
    for (const auto& appF : m_appFunctions) {
        if (!OpenEnclaveSupportedLibFunctions::get().supportsFunction(appF.getName())) {
            untrustedScope->addFunction(generateFunctionForEdl(appF, false));
        }
    }
    enclaceScope->addSubscope(trustedScope);
    enclaceScope->addSubscope(untrustedScope);
    m_edlFile.addSubscope(enclaceScope);
}

void OpenEnclaveCodeGenerator::generateEnclaveFile()
{
    m_enclaveFile.setName(m_prefix + "_enclave.c");
    m_enclaveFile.setHeader(false);
    m_enclaveFile.addInclude("<openenclave/enclave.h>");
    const std::string trusted_edl_name = "\"" + m_prefix + "_t.h\"";
    m_enclaveFile.addInclude(trusted_edl_name);
    m_enclaveFile.addInclude("<stdio.h>");

    for (const auto& appF : m_appFunctions) {
        if (!OpenEnclaveSupportedLibFunctions::get().supportsFunction(appF.getName())) {
            m_enclaveFile.addFunction(generateOcallWrapper(appF));
        }
    }

    for (const auto& enclaveF : m_enclaveFunctions) {
        m_enclaveFile.addFunction(generateExternForFunction(enclaveF));
	m_enclaveFile.addFunction(generateCallWrapper(enclaveF, true));
    }

}

void OpenEnclaveCodeGenerator::generateAppDriverFile()
{
    m_appDriverFile.setName(m_prefix + "_app.c");
    m_appDriverFile.setHeader(false);
    const std::string untrusted_edl_name = "\"" + m_prefix + "_u.h\"";
    m_appDriverFile.addInclude(untrusted_edl_name);
    // app_util.h has enclave creation and termination code
    m_appDriverFile.addInclude("\"app_utils.h\"");
    m_appDriverFile.addInclude("<stdio.h>");

    Function appMainF("app_main");
    appMainF.setIsExtern(true);
    appMainF.setReturnType(Type{"int", "", false, false});
    m_appDriverFile.addFunction(appMainF);

    m_appDriverFile.addFunction(generateMain());
    for (const auto& secureF : m_enclaveFunctions) {
        m_appDriverFile.addFunction(generateEcallWrapper(secureF));
    }

    for (const auto& appF : m_appFunctions) {
        if (!OpenEnclaveSupportedLibFunctions::get().supportsFunction(appF.getName())) {
            m_appDriverFile.addFunction(generateExternForFunction(appF));
            m_appDriverFile.addFunction(generateCallWrapper(appF, false));
        }
    }
}

}

