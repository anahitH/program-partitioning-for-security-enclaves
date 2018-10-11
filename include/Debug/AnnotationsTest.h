#pragma once

#include "llvm/Support/CommandLine.h"
#include <vector>

namespace vazgen {
class Annotation;
class Logger;
}

namespace debug {

extern llvm::cl::opt<std::string> Annot;
extern llvm::cl::opt<std::string> Annotations;
extern llvm::cl::opt<std::string> ExpectedAnnotations;

/// Utility class for testing annotation parsers'
class AnnotationsTest {
public:
    struct ExpectedAnnotation
    {
        std::string m_functionName;
        std::vector<int> m_arguments;
        bool m_return = false;
    };

    using ExpectedAnnotations = std::vector<ExpectedAnnotation>;
    static ExpectedAnnotations parseExpectedAnnotations(const std::string file_name,
                                                        vazgen::Logger& logger);

}; // class AnnotationsTest

bool operator == (const AnnotationsTest::ExpectedAnnotation& annot1, const vazgen::Annotation& annot2);
bool operator != (const AnnotationsTest::ExpectedAnnotation& annot1, const vazgen::Annotation& annot2);
} // namespace debug

