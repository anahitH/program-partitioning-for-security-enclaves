#include "CodeGen/SGXCodeGenerator.h"
#include "CodeGen/SourceFileWriter.h"

#include <sstream>
#include <iostream>

namespace vazgen {

SGXCodeGenerator::SGXCodeGenerator(const std::string& programName,
                                   const Functions& secureFunctions,
                                   const Functions& appFunctions)
    : m_prefix(programName)
    , m_enclaveFunctions(secureFunctions)
    , m_appFunctions(appFunctions)
{
}

void SGXCodeGenerator::generate()
{
    // generate selectors
    generateInterfaceSelectors();
    // generate enclave - appname_enclave.cc
    generateEnclaveRunner();
    // generate app_driver
    generateAppDriver();

    writeGeneratedFiles();
}

void SGXCodeGenerator::generateInterfaceSelectors()
{
    const std::string kSelector = "kSelectorUser";
    int i = 1;
    for (const auto& enclaveF : m_enclaveFunctions) {
        std::string selectorValue = kSelector + " + " + std::to_string(i++);
        std::string selectorName = "k" + enclaveF.getName() + "EnclaveSelector";
        m_ecallSelectors.insert(std::make_pair(enclaveF.getName(), FunctionSelector{enclaveF.getName(), selectorName, selectorValue}));
    }

    i = 0;
    for (const auto& appF : m_appFunctions) {
        std::string selectorValue = kSelector + " + " + std::to_string(i++);
        std::string selectorName = "k" + appF.getName() + "OCallHandler";
        m_ocallSelectors.insert(std::make_pair(appF.getName(), FunctionSelector{appF.getName(), selectorName, selectorValue}));
    }

    m_interfaceSelectorsFile.setName("interface_selectors.h");
    m_interfaceSelectorsFile.setHeader(true);
    m_interfaceSelectorsFile.addMacro("#pragma once");
    m_interfaceSelectorsFile.addInclude("<cstdint>");
    m_interfaceSelectorsFile.addInclude("\"asylo/platform/primitives/primitives.h\"");

    SourceScope::ScopeType asyloNamespace(new SourceScope("asylo"));
    SourceScope::ScopeType primitivesNamespace(new SourceScope("primitives"));

    for (const auto& [fName, selector] : m_ecallSelectors) {
        Variable global = {Type{"uint64_t", "constexpr", false, false}, selector.m_selectorName};
        primitivesNamespace->addGlobalVariable(std::make_pair(global, selector.m_selectorValue));
    }
    // separately register selector for abort function
    Variable abortSelector = {Type{"uint64_t", "constexpr", false, false}, "kAbortEnclaveSelector"};
    primitivesNamespace->addGlobalVariable(std::make_pair(abortSelector, kSelector + std::to_string(m_ecallSelectors.size())));

    for (const auto& [fname, selector] : m_ocallSelectors) {
        Variable global = {Type{"uint64_t", "constexpr", false, false}, selector.m_selectorName};
        primitivesNamespace->addGlobalVariable(std::make_pair(global, selector.m_selectorValue));
    }
    asyloNamespace->addSubscope(primitivesNamespace);
    m_interfaceSelectorsFile.addSubscope(asyloNamespace);
}

void SGXCodeGenerator::generateEnclaveRunner()
{
    m_enclaveFile.setName(m_prefix + "_enclave.cc");
    m_enclaveFile.setHeader(false);
    m_enclaveFile.addInclude("\"" + m_interfaceSelectorsFile.getName() + "\"");
    m_enclaveFile.addInclude("\"asylo/platform/primitives/extent.h\"");
    m_enclaveFile.addInclude("\"asylo/platform/primitives/primitive_status.h\"");
    m_enclaveFile.addInclude("\"asylo/platform/primitives/trusted_primitives.h\"");
    m_enclaveFile.addInclude("\"asylo/util/status_macro.h\"");

    // declaration for actual enclave functions
    for (auto function : m_enclaveFunctions) {
        function.setIsExtern(true);
        m_enclaveFile.addFunction(function);
    }
    SourceScope::ScopeType asyloNamespace(new SourceScope("asylo"));
    SourceScope::ScopeType primitivesNamespace(new SourceScope("primitives"));
    SourceScope::ScopeType unnamedNamespace(new SourceScope(""));

    generateEnclaveAbortFunction(unnamedNamespace);
    generateEnclaveEcalls(unnamedNamespace);
    const auto& ocallWrappers = generateAppFunctionsInEnclave();
    for (const auto& ocallWrapper : ocallWrappers) {
        primitivesNamespace->addFunction(ocallWrapper);
    }
    generateAsyloEnclaveInitFunction(primitivesNamespace);
    generateAsyloEnclaveFiniFunction(primitivesNamespace);

    primitivesNamespace->addSubscope(unnamedNamespace);
    asyloNamespace->addSubscope(primitivesNamespace);
    m_enclaveFile.addSubscope(asyloNamespace);
}

void SGXCodeGenerator::generateAppDriver()
{
    m_appDriverFile.setName(m_prefix + "_driver.cc");
    m_appDriverFile.setHeader(false);
    m_appDriverFile.addInclude("\"" + m_interfaceSelectorsFile.getName() + "\"");
    m_appDriverFile.addInclude("\"gflags/gflags.h\"");
    m_appDriverFile.addInclude("\"asylo/platform/primitives/extent.h\"");
    m_appDriverFile.addInclude("\"asylo/platform/primitives/sim/untrusted_sim.h\"");
    m_appDriverFile.addInclude("\"asylo/platform/primitives/untrusted_primitives.h\"");
    m_appDriverFile.addInclude("\"asylo/platform/primitives/primitive_status.h\"");
    m_appDriverFile.addInclude("\"asylo/platform/primitives/util/dispatch_table.h\"");
    m_appDriverFile.addInclude("\"asylo/platform/primitives/trusted_primitives.h\"");
    m_appDriverFile.addInclude("\"asylo/util/status_macro.h\"");

    m_appDriverFile.addMacro("DEFINE_string(enclave_path, \"\", \"Path to enclave binary\");");
    Function appMainF("app_main");
    appMainF.setIsExtern(true);
    appMainF.setReturnType(Type{"int", "", false, false});
    m_appDriverFile.addFunction(appMainF);

    // declaration for actual enclave functions
    for (auto function : m_appFunctions) {
        function.setIsExtern(true);
        m_appDriverFile.addFunction(function);
    }
    // generate main
    generateAppDriverMain();

    SourceScope::ScopeType asyloNamespace(new SourceScope("asylo"));
    SourceScope::ScopeType primitivesNamespace(new SourceScope("primitives"));
    primitivesNamespace->addGlobalVariable(std::make_pair(Variable{Type{"std::shared_ptr<EnclaveClient>", "", false, false}, "client"}, ""));

    // generate ocall registration
    generateOCallRegistration(primitivesNamespace);
    // generate ocalls
    generateOCalls(primitivesNamespace);
    // generate ecall functions
    const auto& ecallWrappers = generateEnclaveFunctionsInDriver();
    for (const auto& ecall : ecallWrappers) {
        primitivesNamespace->addFunction(ecall);
    }
    asyloNamespace->addSubscope(primitivesNamespace);
    m_appDriverFile.addSubscope(asyloNamespace);
}

void SGXCodeGenerator::generateEnclaveAbortFunction(SourceScope::ScopeType& inScope)
{
    Function enclaveAbortFunction("Abort");
    enclaveAbortFunction.setReturnType(Type{"PrimitiveStatus", "", false, false});
    enclaveAbortFunction.addParam(Variable {Type{"void", "", true, false}, "context"});
    enclaveAbortFunction.addParam(Variable {Type{"TrustedParameterStack", "", true, false}, "params"});
    enclaveAbortFunction.addBody("TrustedPrimitives::BestEffortAbort(\"Aborting enclave\")");
    enclaveAbortFunction.addBody("return PrimitiveStatus::OkStatus()");
    inScope->addFunction(enclaveAbortFunction);
}

void SGXCodeGenerator::generateEnclaveEcalls(SourceScope::ScopeType& inScope)
{
    for (const auto& enclaveF : m_enclaveFunctions) {
        generateEnclaveEcall(enclaveF, inScope);
    }
}

std::vector<Function> SGXCodeGenerator::generateAppFunctionsInEnclave()
{
    std::vector<Function> appFunctionOcalls;
    for (const auto& appF : m_appFunctions) {
        const auto& ocallWrapper = generateAppFunctionWrapperInEnclave(appF);
        appFunctionOcalls.push_back(ocallWrapper);
        generateAppFunctionInEnclave(ocallWrapper, appF);
    }
    return appFunctionOcalls;
}

void SGXCodeGenerator::generateEnclaveEcall(const Function& enclaveF, SourceScope::ScopeType& inScope)
{
    Function ecallF("secure_" + enclaveF.getName());
    ecallF.setReturnType(Type{"PrimitiveStatus"});
    ecallF.addParam(Variable {Type{"void", "", true, false}, "context"});
    ecallF.addParam(Variable {Type{"TrustedParameterStack", "", true, false}, "params"});

    // body
    // parsing passed arguments, assuming they where pushed in the call order, then pop in the reverse order
    std::vector<std::string> call_params(enclaveF.getParams().size());
    int i = enclaveF.getParams().size();
    for (auto r_it = enclaveF.getParams().rbegin(); r_it != enclaveF.getParams().rend(); ++r_it) {
        std::stringstream getParamStr;
        getParamStr << r_it->m_name << " ";
        std::string paramName = r_it->m_name + "_param";
        call_params[--i] = paramName;
        getParamStr << paramName << " = " << "params->Pop<" << r_it->m_type.m_name << ">()";
        ecallF.addBody(getParamStr.str());
    }
    // insertin call to the actual function
    std::string callStr = enclaveF.getCallAsString(call_params);
    std::string returnVal;
    if (!enclaveF.isVoidReturn()) {
        returnVal = "returnVal";
        std::stringstream returnStr;
        returnStr << enclaveF.getReturnType().m_name << " " << returnVal << " = "
                  << callStr;
        ecallF.addBody(returnStr.str());
    } else {
        ecallF.addBody(callStr);
    }
    // setting up return parameters
    // push in the reverse order
    // first goes return value if it exists
    if (!enclaveF.isVoidReturn()) {
        std::stringstream retValPushStr;
        retValPushStr << "*params->PushAlloc<"
                      << enclaveF.getReturnType().m_name << ">() = "
                      << returnVal;
        ecallF.addBody(retValPushStr.str());
    }
    // now parameters
    for (const auto& param : enclaveF.getParams()) {
        std::stringstream paramPushStr;
        paramPushStr << "*params->PushAlloc<" << param.m_type.m_name << ">() = "
                     << param.m_name + "_param";
        ecallF.addBody(paramPushStr.str());
    }
    ecallF.addBody("PrimitiveStatus::OkStatus()");
    inScope->addFunction(ecallF);
}

Function SGXCodeGenerator::generateAppFunctionWrapperInEnclave(const Function appF)
{
    // this is the function that invokes untrusted call
    Function ocallWrapper(appF.getName());
    ocallWrapper.setReturnType(Type{"PrimitiveStatus", "", false, false});
    ocallWrapper.setParams(appF.getParams());
    if (!appF.isVoidReturn()) {
        Type returnType = appF.getReturnType();
        returnType.m_isPtr = true;
        ocallWrapper.addParam(Variable{returnType, "returnVal"});
    }
    ocallWrapper.addBody("TrustedParameterStack params");
    // Push params in the call order
    for (const auto& param : ocallWrapper.getParams()) {
        std::stringstream paramPushStr;
        paramPushStr << "*params.PushAlloc<" << param.m_type.m_name << ">() = ";
        if (param.m_type.m_isPtr) {
            paramPushStr << "*";
        }
        paramPushStr << param.m_name;
        ocallWrapper.addBody(paramPushStr.str());
    }
    // ecall
    const std::string& selector = m_ocallSelectors[appF.getName()].m_selectorName;
    ocallWrapper.addBody("ASYLO_RETURN_IF_ERROR(TrustedPrimitives::UntrustedCall(" + selector + ", &params))");

    // pop returned params
    for (const auto& param : ocallWrapper.getParams()) {
        std::stringstream paramPopStr;
        if (param.m_type.m_isPtr) {
            paramPopStr << "*";
        }
        paramPopStr << param.m_name << " = params.Pop<" << param.m_type.m_name
                    << ">()";
        ocallWrapper.addBody(paramPopStr.str());
    }
    ocallWrapper.addBody("return PrimitiveStatus::OkStatus()");
    return ocallWrapper;
}

void SGXCodeGenerator::generateAppFunctionInEnclave(const Function& ocallWrapper, const Function& appF)
{
    // this is the function that will be envoked by enclave functions and will redirect the call to app
    Function ocallF = appF;
    // generateBody
    std::string returnVal = "returnVal";
    std::vector<std::string> callParams;
    for (const auto& param : appF.getParams()) {
        callParams.push_back(param.m_name);
    }
    if (!appF.isVoidReturn()) {
        ocallF.addBody(appF.getReturnType().getAsString() + " " + returnVal);
        callParams.push_back(returnVal);
    }
    const std::string& callStr = ocallWrapper.getCallAsString(callParams);
    ocallF.addBody("asylo::primitives::" + callStr);
    ocallF.addBody("return returnVal");
    m_enclaveFile.addFunction(ocallF);
}

void SGXCodeGenerator::generateAsyloEnclaveInitFunction(SourceScope::ScopeType& inScope)
{
    Function enclaveInitF("asylo_enclave_init");
    enclaveInitF.setIsExtern(true);
    enclaveInitF.setDeclPrefix("\"C\"");
    enclaveInitF.setReturnType(Type{"PrimitiveStatus"});

    for (const auto& [fname, ecall] : m_ecallSelectors) {
        std::stringstream ecallReg;
        ecallReg << " ASYLO_RETURN_IF_ERROR(TrustedPrimitives::RegisterEntryHandler("
                 << ecall.m_selectorName << ", EntryHandler{" << "secure_" << ecall.m_functionName << "}))";
        enclaveInitF.addBody(ecallReg.str());
    }
    enclaveInitF.addBody("return PrimitiveStatus::OkStatus()");
    inScope->addFunction(enclaveInitF);
}

void SGXCodeGenerator::generateAsyloEnclaveFiniFunction(SourceScope::ScopeType& inScope)
{
    Function enclaveFiniF("asylo_enclave_fini");
    enclaveFiniF.setIsExtern(true);
    enclaveFiniF.setDeclPrefix("\"C\"");
    enclaveFiniF.setReturnType(Type{"PrimitiveStatus"});
    enclaveFiniF.addBody("return PrimitiveStatus::OkStatus()");
    inScope->addFunction(enclaveFiniF);
}

void SGXCodeGenerator::generateAppDriverMain()
{
    Function mainF("main");
    mainF.setReturnType(Type{"int", "", false, false});
    mainF.addParam(Variable{Type{"int", "", false, false}, "argc"});
    mainF.addParam(Variable{Type{"char", "", true, true}, "argv"});

    mainF.addBody("google::ParseCommandLineFlags(&argc, &argv, true)");
    mainF.addBody("asylo::primitives::registerOCalls()");
    mainF.addBody("app_main()");
    mainF.addBody("return 0");

    m_appDriverFile.addFunction(mainF);
}

void SGXCodeGenerator::generateOCallRegistration(SourceScope::ScopeType& inScope)
{
    Function OCallRegF("registerOCalls");
    OCallRegF.setReturnType(Type{"Status", "", false, false});
    OCallRegF.addBody("ASYLO_ASSIGN_OR_RETURN(client, LoadEnclave<SimBackend>(FLAGS_enclave_path, absl::make_unique<DispatchTable>()));");
    OCallRegF.addBody("Status status");
    for (const auto& appF : m_appFunctions) {
        std::stringstream regStr;
        regStr << "status = client->exit_call_provider()->RegisterExitHandler("
               << m_ocallSelectors[appF.getName()].m_selectorName << ", ExitHandler{"
               << "app_" << appF.getName() << "})";
        OCallRegF.addBody(regStr.str());
    }
    inScope->addFunction(OCallRegF);
}

void SGXCodeGenerator::generateOCalls(SourceScope::ScopeType inScope)
{
    for (const auto& appF : m_appFunctions) {
        generateOCall(appF, inScope);
    }
}

void SGXCodeGenerator::generateOCall(const Function& appF, SourceScope::ScopeType inScope)
{
    Function ocallF("app_" + appF.getName());
    ocallF.setReturnType(Type{"Status"});
    ocallF.addParam(Variable {Type{"std::shared_ptr<EnclaveClient>", "", false, false}, "client"});
    ocallF.addParam(Variable {Type{"void", "", true, false}, "context"});
    ocallF.addParam(Variable {Type{"UntrustedParameterStack", "", true, false}, "params"});

    // body
    // parsing passed arguments, assuming they where pushed in the call order, then pop in the reverse order
    std::vector<std::string> call_params(appF.getParams().size());
    int i = appF.getParams().size();
    for (auto r_it = appF.getParams().rbegin(); r_it != appF.getParams().rend(); ++r_it) {
        std::stringstream getParamStr;
        getParamStr << r_it->m_name << " ";
        std::string paramName = r_it->m_name + "_param";
        call_params[--i] = paramName;
        getParamStr << paramName << " = " << "params->Pop<" << r_it->m_type.m_name << ">()";
        ocallF.addBody(getParamStr.str());
    }
    // insertin call to the actual function
    std::string callStr = appF.getCallAsString(call_params);
    std::string returnVal;
    if (!appF.isVoidReturn()) {
        returnVal = "returnVal";
        std::stringstream returnStr;
        returnStr << appF.getReturnType().m_name << " " << returnVal << " = "
                  << callStr;
        ocallF.addBody(returnStr.str());
    } else {
        ocallF.addBody(callStr);
    }
    // setting up return parameters
    // push in the reverse order
    // first goes return value if it exists
    if (!appF.isVoidReturn()) {
        std::stringstream retValPushStr;
        retValPushStr << "*params->PushAlloc<"
                      << appF.getReturnType().m_name << ">() = "
                      << returnVal;
        ocallF.addBody(retValPushStr.str());
    }
    // now parameters
    for (const auto& param : appF.getParams()) {
        std::stringstream paramPushStr;
        paramPushStr << "*params->PushAlloc<" << param.m_type.m_name << ">() = "
                     << param.m_name + "_param";
        ocallF.addBody(paramPushStr.str());
    }
    ocallF.addBody("PrimitiveStatus::OkStatus()");
    inScope->addFunction(ocallF);
}

std::vector<Function> SGXCodeGenerator::generateEnclaveFunctionsInDriver()
{
    std::vector<Function> ecallWrappers;
    for (const auto& enclaveF : m_enclaveFunctions) {
        const auto& ecallWrapper = generateEcallWrapper(enclaveF);
        ecallWrappers.push_back(ecallWrapper);
        generateEnclaveFunctionInDriver(ecallWrapper, enclaveF);
    }
    return ecallWrappers;
}

Function SGXCodeGenerator::generateEcallWrapper(const Function& enclaveF)
{
    Function ecallWrapper(enclaveF.getName());
    ecallWrapper.setReturnType(Type{"Status", "", false, false});
    ecallWrapper.setParams(enclaveF.getParams());
    if (!enclaveF.isVoidReturn()) {
        Type returnType = enclaveF.getReturnType();
        returnType.m_isPtr = true;
        ecallWrapper.addParam(Variable{returnType, "returnVal"});
    }
    ecallWrapper.addBody("UntrustedParameterStack params");
    // Push params in the call order
    for (const auto& param : ecallWrapper.getParams()) {
        std::stringstream paramPushStr;
        paramPushStr << "*params.PushAlloc<" << param.m_type.m_name << ">() = ";
        if (param.m_type.m_isPtr) {
            paramPushStr << "*";
        }
        paramPushStr << param.m_name;
        ecallWrapper.addBody(paramPushStr.str());
    }
    // ecall
    const std::string& selector = m_ecallSelectors[enclaveF.getName()].m_selectorName;
    ecallWrapper.addBody("auto status = client->EnclaveCall(" + selector + ", &params)");

    // pop returned params
    for (const auto& param : ecallWrapper.getParams()) {
        std::stringstream paramPopStr;
        if (param.m_type.m_isPtr) {
            paramPopStr << "*";
        }
        paramPopStr << param.m_name << " = params.Pop<" << param.m_type.m_name
                    << ">()";
        ecallWrapper.addBody(paramPopStr.str());
    }
    ecallWrapper.addBody("return Status::OkStatus()");
    return ecallWrapper;
}

void SGXCodeGenerator::generateEnclaveFunctionInDriver(const Function& ecallWrapper, const Function& enclaveF)
{
    Function ecallF = enclaveF;
    // generateBody
    std::string returnVal = "returnVal";
    std::vector<std::string> callParams;
    for (const auto& param : enclaveF.getParams()) {
        callParams.push_back(param.m_name);
    }
    if (!enclaveF.isVoidReturn()) {
        ecallF.addBody(enclaveF.getReturnType().getAsString() + " " + returnVal);
        callParams.push_back(returnVal);
    }
    const std::string& callStr = ecallWrapper.getCallAsString(callParams);
    ecallF.addBody("asylo::primitives::" + callStr);
    ecallF.addBody("return returnVal");
    m_appDriverFile.addFunction(ecallF);
}

void SGXCodeGenerator::writeGeneratedFiles()
{
    SourceFileWriter selectorsFileWriter(m_interfaceSelectorsFile);
    selectorsFileWriter.write();

    SourceFileWriter enclaveFileWriter(m_enclaveFile);
    enclaveFileWriter.write();

    SourceFileWriter appDriverFileWriter(m_appDriverFile);
    appDriverFileWriter.write();
}

} // namespace vazgen

