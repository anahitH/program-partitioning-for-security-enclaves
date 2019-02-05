#include "clang/ASTMatchers/ASTMatchers.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"
#include "clang/Frontend/FrontendActions.h"
#include "clang/Tooling/CommonOptionsParser.h"
#include "clang/Tooling/Tooling.h"
#include "llvm/Support/CommandLine.h"

#include "ClangTools/ProtoFileGenerator.h"
#include "CodeGen/ProtoFileWriter.h"

#include <fstream>
#include <iostream>
#include <unordered_set>

using namespace clang;
using namespace clang::ast_matchers;
using namespace clang::tooling;

namespace vazgen {

std::unordered_set<std::string> parseFunctions(const std::string& fileName)
{
    std::unordered_set<std::string> functions;
    std::ifstream fileStream(fileName);
    std::string function;
    while (std::getline(fileStream, function)) {
        functions.insert(function);
    }
    return functions;
}

DeclarationMatcher functionMatcher = functionDecl().bind("functionDecl");
static llvm::cl::OptionCategory ProtoFileGenTool("proto-gen options");
static llvm::cl::extrahelp CommonHelp(CommonOptionsParser::HelpMessage);
static llvm::cl::extrahelp MoreHelp("\nMore help text...\n");
static llvm::cl::opt<std::string> functionFile("functions", llvm::cl::cat(ProtoFileGenTool));

class FunctionFinder : public MatchFinder::MatchCallback
{
public:
    FunctionFinder(const std::unordered_set<std::string>& functions)
        : m_functions(functions)
    {
    }

    const std::unordered_set<const FunctionDecl*>& getFunctions()
    {
        return m_foundFunctions;
    }

public:
    virtual void run(const MatchFinder::MatchResult &Result) override
    {
        const FunctionDecl* decl = Result.Nodes.getNodeAs<clang::FunctionDecl>("functionDecl");
        if (!decl) {
            return;
        }
        if (m_functions.find(decl->getNameInfo().getName().getAsString()) != m_functions.end()) {
            m_foundFunctions.insert(decl);
        }
    }

private:
    const std::unordered_set<std::string>& m_functions;
    std::unordered_set<const FunctionDecl*> m_foundFunctions;
}; // class FunctionFinder


} // namespace vazgen

int main(int argc, const char* argv[])
{
    std::cout << "Start\n";
    CommonOptionsParser OptionsParser(argc, argv, vazgen::ProtoFileGenTool);
    ClangTool Tool(OptionsParser.getCompilations(),
                 OptionsParser.getSourcePathList());
    const auto& functions = vazgen::parseFunctions(vazgen::functionFile.getValue());

    vazgen::FunctionFinder functionFinder(functions);
    MatchFinder matchFinder;
    matchFinder.addMatcher(vazgen::functionMatcher, &functionFinder);

    Tool.run(newFrontendActionFactory(&matchFinder).get());

    const auto& functionDecls = functionFinder.getFunctions();
    vazgen::ProtoFileGenerator protoFileGen(functionDecls, "secure_service");
    protoFileGen.generate();

    vazgen::ProtoFileWriter protoWriter("secure_service.proto", protoFileGen.getProtoFile());
    protoWriter.write();

    return 0;
}

