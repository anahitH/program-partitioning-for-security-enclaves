#include "Debug/AnnotationsTest.h"
#include "Annotation.h"
#include "Logger.h"

#include <algorithm>
#include <fstream>

namespace debug {

llvm::cl::opt<std::string> Annot(
    "annotation",
    llvm::cl::desc("Find object with given annotation"),
    llvm::cl::value_desc("annotation"));

llvm::cl::opt<std::string> Annotations(
    "annotations",
    llvm::cl::desc("Json file containing annotations"),
    llvm::cl::value_desc("annotation"));

llvm::cl::opt<std::string> ExpectedAnnotations(
    "expected-annotations",
    llvm::cl::desc("Expected annotated functions"),
    llvm::cl::value_desc("annotation"));


bool isDigit(const std::string& str)
{
    for (auto c : str) {
        if (!std::isdigit(c)) {
            return false;
        }
    }
    return true;
}

AnnotationsTest::ExpectedAnnotations
AnnotationsTest::parseExpectedAnnotations(const std::string file_name,
                                          vazgen::Logger& logger)
{
    ExpectedAnnotations expectedAnnotations;
    std::ifstream functions_strm(file_name);
    if (!functions_strm.is_open()) {
        logger.error("Failed to open file");
        return expectedAnnotations;
    }
    std::string annotation_str;
    while (getline(functions_strm, annotation_str)) {
        if (annotation_str.empty()) {
            continue;
        }
        std::istringstream iss(annotation_str);
        const std::vector<std::string> tokens{std::istream_iterator<std::string>{iss},
                                              std::istream_iterator<std::string>{}};
        if (tokens.empty()) {
            logger.error("Wrong line in expected annotations file " + annotation_str + "\n");
            continue;
        }
        ExpectedAnnotation annot;
        annot.m_functionName = tokens[0];
        for (int i = 1; i < tokens.size(); ++i) {
            if (isDigit(tokens[i])) {
                annot.m_arguments.push_back(std::stoi(tokens[i]));
            } else if (tokens[i] == "return") {
                annot.m_return = true;
            } else {
                logger.warn("Wrong token in expected annotations file line " + annotation_str + " " + tokens[i] + "\n");
            }
        }
        expectedAnnotations.push_back(annot);
    }
    return expectedAnnotations;
}

bool operator == (const AnnotationsTest::ExpectedAnnotation& annot1, const vazgen::Annotation& annot2)
{
    if (annot1.m_functionName != annot2.getFunction()->getName()) {
        return false;
    }
    if (annot1.m_return != annot2.isReturnAnnotated()) {
        return false;
    }
    auto res = std::mismatch(annot1.m_arguments.begin(),
                             annot1.m_arguments.end(),
                             annot2.getAnnotatedArguments().begin());
    return (res.first == annot1.m_arguments.end() && res.second == annot2.getAnnotatedArguments().end());

}

bool operator != (const AnnotationsTest::ExpectedAnnotation& annot1, const vazgen::Annotation& annot2)
{
    return !(annot1 == annot2);
}

}

