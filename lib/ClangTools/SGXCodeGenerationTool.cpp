#include "clang/ASTMatchers/ASTMatchers.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"
#include "clang/Frontend/FrontendActions.h"
#include "clang/Tooling/CommonOptionsParser.h"
#include "clang/Tooling/Tooling.h"
#include "llvm/Support/CommandLine.h"

#include "Utils/Logger.h"
#include "ClangTools/ClangToolUtils.h"
#include "CodeGen/CodeGenUtils.h"
#include "CodeGen/Function.h"

#include <fstream>
#include <iostream>
#include <unordered_set>

using namespace clang;
using namespace clang::ast_matchers;
using namespace clang::tooling;


namespace vazgen {

const std::string FILE_DELIM = "/";
DeclarationMatcher functionMatcher = functionDecl().bind("functionDecl");

static llvm::cl::OptionCategory SGXCodeGenTool("SGX code generation options");
static llvm::cl::extrahelp CommonHelp(CommonOptionsParser::HelpMessage);
static llvm::cl::extrahelp MoreHelp("\nMore help text...\n");
static llvm::cl::opt<std::string> functionStats("partition-stats",
                                                llvm::cl::desc("Statistics json file containing partition info for SGX code generation."),
                                                llvm::cl::ZeroOrMore, llvm::cl::cat(SGXCodeGenTool));
static llvm::cl::opt<std::string> protoName("prefix",
                                            llvm::cl::desc("Prefix to use when generating SGX files"),
                                            llvm::cl::ZeroOrMore, llvm::cl::cat(SGXCodeGenTool));

class FunctionFinder : public MatchFinder::MatchCallback
{
public:
    FunctionFinder(const std::unordered_set<std::string>& functions)
        : m_functionsNames(functions)
    {
    }

    std::vector<Function> getFunctions()
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
        if (m_functionsNames.find(decl->getNameInfo().getName().getAsString()) == m_functionsNames.end()) {
            return;
        }
        // TODO: test this with memcached
        if (decl->isDefined() && !decl->isThisDeclarationADefinition()) {
            return;
        }
        if (decl->isDefined()) {
            m_functionsNames.erase(decl->getNameInfo().getName().getAsString());
        }
        m_functionDecls[decl->getName()] = decl;
        m_foundFunctions.push_back(getFunctionFromClangDecl(decl));
    }

private:
    Function getFunctionFromClangDecl(const FunctionDecl* Fdecl)
    {
        Function F(Fdecl->getName().str());
        for (unsigned i = 0; i < Fdecl->getNumParams(); ++i) {
            auto* paramDecl = Fdecl->getParamDecl(i);
            const auto& paramName = paramDecl->getName();
            const auto& paramType = getTypeFromClangType(&*paramDecl->getType());
            F.addParam(Variable{paramType, paramName});
        }
        if (!Fdecl->getReturnType()->isVoidType()) {
            F.setReturnType(Type{"void", "", false, false});
        } else {
            F.setReturnType(getTypeFromClangType(&*Fdecl->getReturnType()));
        }
        return F;
    }

    vazgen::Type getTypeFromClangType(const clang::Type* clangT)
    {
        vazgen::Type type;
        if (auto* ptrType = llvm::dyn_cast<clang::PointerType>(clangT)) {
            type = getTypeFromClangType(&*ptrType->getPointeeType());
            type.m_isPtr = true;
            return type;
        }
        if (auto* arrayType = llvm::dyn_cast<clang::ArrayType>(clangT)) {
            auto* elementType = &*arrayType->getElementType();
            type = getTypeFromClangType(elementType);
            type.m_isArray = true;
            return type;
        }
        type.m_name = ClangToolUtils::getTypeName(clangT);
        type.m_isPtr = false;
        type.m_isArray = false;
        // TODO: fill qualifier
        return type;
    }

private:
    std::unordered_set<std::string> m_functionsNames;
    std::vector<Function> m_foundFunctions;
    std::unordered_map<std::string, const FunctionDecl*> m_functionDecls;
}; // class FunctionFinder

}

int main(int argc, const char* argv[])
{
    CommonOptionsParser OptionsParser(argc, argv, vazgen::SGXCodeGenTool);
    vazgen::Logger logger("code-gen");
    logger.setLevel(vazgen::Logger::ERR);
    if (vazgen::functionStats.empty()) {
        logger.error("No file is specified for functions");
        return 0;
    }
    const auto& [secureFunctionNames, insecureFunctionNames] = vazgen::CodeGenUtils::parseFunctionsFromStats(vazgen::functionStats.getValue(), logger);
    ClangTool Tool(OptionsParser.getCompilations(), {OptionsParser.getSourcePathList()});

    vazgen::FunctionFinder secureFunctionFinder(secureFunctionNames);
    MatchFinder matchFinder;
    matchFinder.addMatcher(vazgen::functionMatcher, &secureFunctionFinder);
    Tool.run(newFrontendActionFactory(&matchFinder).get());
    const auto& secureFunctions = secureFunctionFinder.getFunctions();

    vazgen::FunctionFinder insecureFunctionFinder(insecureFunctionNames);
    matchFinder.addMatcher(vazgen::functionMatcher, &insecureFunctionFinder);
    Tool.run(newFrontendActionFactory(&matchFinder).get());
    const auto& insecureFunctions = insecureFunctionFinder.getFunctions();

    return 0;
} 
