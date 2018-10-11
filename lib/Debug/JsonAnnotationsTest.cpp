#include "JsonAnnotationParser.h"
#include "Debug/AnnotationsTest.h"

#include "Logger.h"

#include "llvm/Pass.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/IR/Module.h"
#include "llvm/PassRegistry.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Transforms/IPO/PassManagerBuilder.h"

#include <cassert>
#include <fstream>
#include <unordered_set>

namespace debug {

class JsonAnnotationTestPass : public llvm::ModulePass
{
public:
    static char ID;
    JsonAnnotationTestPass()
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
        if (Annotations.empty()) {
            logger.error("No json file is provided with annotations\n");
            return false;
        }
        if (ExpectedAnnotations.empty()) {
            logger.error("No expected annotations are given to check against\n");
            return false;
        }
        const auto& expectedAnnotations = AnnotationsTest::parseExpectedAnnotations(ExpectedAnnotations, logger);
        vazgen::JsonAnnotationParser annotations_parser(&M, Annotations, logger);
        const auto& annotatedObjects = annotations_parser.getAnnotations(Annot);
        checkAnnotations(expectedAnnotations, annotatedObjects, logger);

        //for (auto& annotation : annotatedObjects) {
        //    llvm::dbgs() << "F: " << annotation.getFunction()->getName() << "\n";
        //    llvm::dbgs() << "Return: " << annotation.isReturnAnnotated() << "\n";
        //    for (auto& arg : annotation.getAnnotatedArguments()) {
        //        llvm::dbgs() << "Arg: " << arg << "\n";
        //    }
        //}
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
    
}; // JsonAnnotationTestPass

char JsonAnnotationTestPass::ID = 0;
static llvm::RegisterPass<JsonAnnotationTestPass> X("test-json-annotations","Test ModuleAnnotationParser");

} // namespace debug

