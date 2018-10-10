#include "ModuleAnnotationParser.h"
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

static llvm::cl::opt<std::string> Annot(
    "annotation",
    llvm::cl::desc("Find object with given annotation"),
    llvm::cl::value_desc("annotation"));

static llvm::cl::opt<std::string> ExpectedAnnotations(
    "expected-annotations",
    llvm::cl::desc("Expected annotated functions"),
    llvm::cl::value_desc("annotation"));

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
        const auto& expectedAnnotations = parseExpectedAnnotations(ExpectedAnnotations, logger);
        vazgen::ModuleAnnotationParser annotations_parser(&M, logger);
        const auto& annotatedObjects = annotations_parser.getAnnotations(Annot);
        checkAnnotations(expectedAnnotations, annotatedObjects, logger);
        return false;
    }

private:
    std::unordered_set<std::string> parseExpectedAnnotations(const std::string& filename,
                                                             vazgen::Logger& logger)
    {
        std::unordered_set<std::string> expectedAnnotatedFunctions;
        std::ifstream functions_strm(filename);
        if (!functions_strm.is_open()) {
            logger.error("Failed to open file");
            return expectedAnnotatedFunctions;
        }
        std::string name;
        while (!functions_strm.eof()) {
            functions_strm >> name;
            expectedAnnotatedFunctions.insert(name);
        }
        return expectedAnnotatedFunctions;
    }

    void checkAnnotations(const std::unordered_set<std::string>& expectedAnnotations,
                          const vazgen::AnnotationParser::Annotations& annotations,
                          vazgen::Logger& logger)
    {
        assert((annotations.size() == expectedAnnotations.size())
                && "Mismatch between expectedAnnotations and parsed annotations");
        for (auto& item : annotations) {
            assert((expectedAnnotations.find(item.getFunction()->getName()) != expectedAnnotations.end())
                    && "Mismatch between expectedAnnotations and parsed annotations");
        }
    }
    
}; // ModuleAnnotationTestPass

char ModuleAnnotationTestPass::ID = 0;
static llvm::RegisterPass<ModuleAnnotationTestPass> X("test-annotations","Test ModuleAnnotationParser");

} // namespace debug

