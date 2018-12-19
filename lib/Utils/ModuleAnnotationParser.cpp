#include "Utils/ModuleAnnotationParser.h"
#include "Utils/Logger.h"

#include "llvm/IR/Module.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Constants.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Support/Debug.h"

namespace vazgen {

ModuleAnnotationParser::ModuleAnnotationParser(llvm::Module* module,
                                               Logger& logger)
    : m_module(module)
    , m_logger(logger)
{
}

void ModuleAnnotationParser::parseAnnotations()
{
    m_parsed = true;
    auto global_annos = m_module->getNamedGlobal("llvm.global.annotations");
    if (!global_annos) {
        m_logger.warn("No global annotations to parse");
        return;
    }
    auto annot = llvm::cast<llvm::ConstantArray>(global_annos->getOperand(0));
    for (unsigned i = 0; i < annot->getNumOperands(); ++i) {
        auto e = llvm::cast<llvm::ConstantStruct>(annot->getOperand(i));
        if (auto fn = llvm::dyn_cast<llvm::Function>(e->getOperand(0)->getOperand(0))) {

            auto* str_annotation = llvm::cast<llvm::ConstantDataArray>(
                    llvm::cast<llvm::GlobalVariable>(e->getOperand(1)->getOperand(0))->getOperand(0));
            if (!str_annotation) {
                continue;
            }
            const std::string annotation = str_annotation->getAsCString();
            m_annotations[annotation].insert(Annotation(annotation, fn));
        }
    }
}

} // namespace vazgen

