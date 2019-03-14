#include "CodeGen/SGXCodeGenerator.h"

#include "CodeGen/SourceFileWriter.h"

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
    // generate app_driver

    SourceFileWriter fileWriter(m_interfaceSelectorsFile);
    fileWriter.write();
}

void SGXCodeGenerator::generateInterfaceSelectors()
{
    const std::string kSelector = "kSelectorUser";
    int i = 1;
    for (const auto& enclaveF : m_enclaveFunctions) {
        std::string selectorValue = kSelector + " + " + std::to_string(i++);
        m_ecallSelectorsMap.insert(std::make_pair(enclaveF.getName(), selectorValue));
    }

    i = 0;
    for (const auto& enclaveF : m_appFunctions) {
        std::string selectorValue = kSelector + " + " + std::to_string(i++);
        m_ocallSelectorsMap.insert(std::make_pair(enclaveF.getName(), selectorValue));
    }


    m_interfaceSelectorsFile.setName("interface_selectors.h");
    m_interfaceSelectorsFile.setHeader(true);
    m_interfaceSelectorsFile.addMacro("#pragma once");
    m_interfaceSelectorsFile.addInclude("<cstdint>");
    m_interfaceSelectorsFile.addInclude("\"asylo/platform/primitives/primitives.h\"");
    m_interfaceSelectorsFile.addNamespace("asylo");
    m_interfaceSelectorsFile.addNamespace("primitives");

    for (const auto& [function, selector] : m_ecallSelectorsMap) {
        std::string selectorName = "k" + function + "EnclaveSelector";
        Variable global = {Type{"uint64_t", "constexpr", false, false}, selectorName};
        m_interfaceSelectorsFile.addGlobalVariable(std::make_pair(global, selector));
    }
    for (const auto& [function, selector] : m_ocallSelectorsMap) {
        std::string selectorName = "k" + function + "OcallHandler";
        Variable global = {Type{"uint64_t", "constexpr", false, false}, selectorName};
        m_interfaceSelectorsFile.addGlobalVariable(std::make_pair(global, selector));
    }

}

} // namespace vazgen

