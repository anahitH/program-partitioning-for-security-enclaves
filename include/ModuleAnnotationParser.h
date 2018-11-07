#pragma once

#include "AnnotationParser.h"

#include <unordered_set>
#include <unordered_map>

namespace llvm {
class Module;
class Function;
} // namespace llvm

namespace vazgen {

class Logger;

class ModuleAnnotationParser : public AnnotationParser
{
public:
    ModuleAnnotationParser(llvm::Module* module, Logger& logger);

    ModuleAnnotationParser(const ModuleAnnotationParser& ) = delete;
    ModuleAnnotationParser(ModuleAnnotationParser&& ) = delete;
    ModuleAnnotationParser& operator = (const ModuleAnnotationParser&) = delete;
    ModuleAnnotationParser& operator = (ModuleAnnotationParser&& ) = delete;

public:
    virtual void parseAnnotations() override;

private:
    llvm::Module* m_module;
    Logger& m_logger;
}; // class ModuleAnnotationParser

} // namespace vazgen

