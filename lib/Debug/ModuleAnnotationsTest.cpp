#include "Utils/ModuleAnnotationParser.h"
#include "Debug/AnnotationsTest.h"
#include "Utils/Logger.h"

#include "llvm/Pass.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/IR/Module.h"
#include "llvm/PassRegistry.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Transforms/IPO/PassManagerBuilder.h"

#include <cassert>
#include <fstream>
#include <unordered_set>

namespace debug {

class ModuleAnnotationTestPass : public llvm::ModulePass
{
public:
    static char ID;
    ModuleAnnotationTestPass()
        : llvm::ModulePass(ID)
    {
    }

    bool runOnModule(llvm::Module& M) override
    {
        vazgen::Logger logger("test");
        logger.setLevel(vazgen::Logger::ERR);
        if (Annot.empty()) {
            logger.error("No annotation is given to check for\n");
            return false;
        }
        if (ExpectedAnnotations.empty()) {
            logger.error("No expected annotations are given to check against\n");
            return false;
        }
        const auto& expectedAnnotations = AnnotationsTest::parseExpectedAnnotations(ExpectedAnnotations, logger);
        vazgen::ModuleAnnotationParser annotations_parser(&M, logger);
        const auto& annotatedObjects = annotations_parser.getAnnotations(Annot);
        checkAnnotations(expectedAnnotations, annotatedObjects, logger);
        return false;
    }

private:
    void checkAnnotations(const AnnotationsTest::ExpectedAnnotations& expectedAnnotations,
                          const vazgen::AnnotationParser::Annotations& annotations,
                          vazgen::Logger& logger)
    {
        assert((annotations.size() == expectedAnnotations.size())
                && "Mismatch between expectedAnnotations and parsed annotations");
        for (auto& item : annotations) {
            if (std::find(expectedAnnotations.begin(), expectedAnnotations.end(), item) == expectedAnnotations.end()) {
                assert("Mismatch between expectedAnnotations and parsed annotations" && false);
                return;
            }
        }
    }
    
}; // ModuleAnnotationTestPass

char ModuleAnnotationTestPass::ID = 0;
static llvm::RegisterPass<ModuleAnnotationTestPass> X("test-annotations","Test ModuleAnnotationParser");

} // namespace debug

