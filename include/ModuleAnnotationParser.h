#pragma once

#include <unordered_set>
#include <unordered_map>

namespace llvm {
class Module;
class Function;
} // namespace llvm

namespace vazgen {

class Logger;

class ModuleAnnotationParser
{
public:
    using FunctionSet = std::unordered_set<llvm::Function*>;
    using AnnotatedFunctions = std::unordered_map<std::string, FunctionSet>;

public:
    ModuleAnnotationParser(llvm::Module* module, Logger& logger);

    ModuleAnnotationParser(const ModuleAnnotationParser& ) = delete;
    ModuleAnnotationParser(ModuleAnnotationParser&& ) = delete;
    ModuleAnnotationParser& operator = (const ModuleAnnotationParser&) = delete;
    ModuleAnnotationParser& operator = (ModuleAnnotationParser&& ) = delete;

public:
    const FunctionSet& getAnnotatedFunctions(const std::string& annotation);

private:
    void parseFunctionAnnotations();

private:
    llvm::Module* m_module;
    bool m_parsed;
    Logger& m_logger;
    AnnotatedFunctions m_annotatedFunctions;
}; // class ModuleAnnotationParser

} // namespace vazgen

