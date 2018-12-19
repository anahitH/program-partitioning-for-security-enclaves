#include "JsonAnnotationParser.h"
#include "Logger.h"

#include "nlohmann/json.hpp"

#include "llvm/IR/Module.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Constants.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Support/Debug.h"

#include <fstream>

namespace vazgen {

JsonAnnotationParser::JsonAnnotationParser(llvm::Module* module,
                                           const std::string& annotationFile,
                                           Logger& logger)
    : m_module(module)
    , m_annotationFile(annotationFile)
    , m_logger(logger)
{
}

void JsonAnnotationParser::parseAnnotations()
{
    m_parsed = true;
    using namespace nlohmann;
    std::ifstream ifs (m_annotationFile, std::ifstream::in);
    if (!ifs.is_open()) {
        m_logger.error("Could not open annotations' json file " + m_annotationFile + "\n");
        return;
    }
    json annotations;
    ifs >> annotations;
    m_logger.info("Parsing annotations from json file " + m_annotationFile + "\n");
    for (auto& annot_item : annotations) {
        const std::string annotation_str = annot_item["annotation"];
        const auto& annotated_functions = annot_item["functions"];
        for (auto& annot_f : annotated_functions) {
            const std::string function_name = annot_f["function"];
            auto* function = m_module->getFunction(function_name);
            if (!function) {
                m_logger.error("Can not find function with name " + function_name + "\n");
                continue;
            }
            Annotation annot(annotation_str, function);
            auto annot_args = annot_f.find("arguments");
            if (annot_args != annot_f.end()) {
                for (auto arg : *annot_args) {
                    annot.addAnnotatedArgument(arg);
                }
            }
            auto annot_return = annot_f.find("return");
            if (annot_return != annot_f.end()) {
                annot.setReturnAnnotation(true);
            }
            m_annotations[annotation_str].insert(annot);
        }
    }
}

} // namespace vazgen

