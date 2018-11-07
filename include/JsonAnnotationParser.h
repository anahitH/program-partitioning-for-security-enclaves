#pragma once

#include "AnnotationParser.h"

#include <unordered_set>
#include <unordered_map>

namespace llvm {
class Function;
class Module;
} // namespace llvm

namespace vazgen {

class Logger;

class JsonAnnotationParser : public AnnotationParser
{
public:
    JsonAnnotationParser(llvm::Module* module,
                         const std::string& annotationFile,
                         Logger& logger);

    JsonAnnotationParser(const JsonAnnotationParser& ) = delete;
    JsonAnnotationParser(JsonAnnotationParser&& ) = delete;
    JsonAnnotationParser& operator = (const JsonAnnotationParser&) = delete;
    JsonAnnotationParser& operator = (JsonAnnotationParser&& ) = delete;

public:
    virtual void parseAnnotations() override;

private:
    llvm::Module* m_module;
    const std::string m_annotationFile;
    Logger& m_logger;
}; // class JsonAnnotationParser

} // namespace vazgen

