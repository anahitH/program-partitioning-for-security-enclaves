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
#include <unordered_map>

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
DeclarationMatcher structMatcher = recordDecl().bind("recordDecl");
DeclarationMatcher enumMatcher = enumDecl().bind("enumDecl");
DeclarationMatcher typedefMatcher = typedefDecl().bind("typedefDecl");
static llvm::cl::OptionCategory ProtoFileGenTool("proto-gen options");
static llvm::cl::extrahelp CommonHelp(CommonOptionsParser::HelpMessage);
static llvm::cl::extrahelp MoreHelp("\nMore help text...\n");
static llvm::cl::opt<std::string> functionFile("functions", llvm::cl::cat(ProtoFileGenTool));

class StructFinder : public MatchFinder::MatchCallback
{
public:
    using Structs = ProtoFileGenerator::Structs;

public:
    const Structs& getStructs() const
    {
        return m_structs;
    }

public:
    virtual void run(const MatchFinder::MatchResult &Result) override
    {
        const auto* decl = Result.Nodes.getNodeAs<clang::RecordDecl>("recordDecl");
        if (!decl) {
            return;
        }
        SourceManager &srcMgr = Result.Context->getSourceManager();
        const std::string src = srcMgr.getFilename(decl->getLocation()).str();
        // TODO: check for files given to the tool
        if (src != "/home/anahitik/TUM/Thesis/program-partitioning-for-security-enclaves/test/proto-generator/snake/snake.h") {
            return;
        }
        if (!decl->isStruct()) {
            return;
        }
        //llvm::dbgs() << "Record Decl " << *decl << "\n";
        ProtoFileGenerator::Struct decl_struct;
        std::string structName;
        auto* typedefdecl = decl->getTypedefNameForAnonDecl();
        if (typedefdecl) {
            //llvm::dbgs() << "name at typedef " << *typedefdecl << "\n";
            structName = typedefdecl->getName();
        } else {
            //llvm::dbgs() << "Is named here\n";
            structName = decl->getName();
        }
        decl_struct.m_name = structName;
        auto* structDecl = llvm::dyn_cast<RecordDecl>(decl);
        for (auto it = structDecl->field_begin(); it != structDecl->field_end(); ++it) {
            ProtoFileGenerator::Struct::Field field = {&*it->getType(), it->getName().str()};
            decl_struct.m_fields.push_back(field);
            //const std::string& fieldName = it->getName().str();
            //llvm::dbgs() << "Field " << fieldName << "\n";
        }
        m_structs.insert(std::make_pair(structName, decl_struct));
    }

private:
    Structs m_structs;  
};

class EnumFinder : public MatchFinder::MatchCallback
{
public:
    using Enums = ProtoFileGenerator::Enums;

public:
    const Enums& getEnums() const
    {
        return m_enums;
    }

public:
    virtual void run(const MatchFinder::MatchResult &Result) override
    {
        // TODO: collect structs defined in given source code only
        const auto* decl = Result.Nodes.getNodeAs<clang::EnumDecl>("enumDecl");
        if (!decl) {
            return;
        }
        SourceManager &srcMgr = Result.Context->getSourceManager();
        const std::string src = srcMgr.getFilename(decl->getLocation()).str();
        if (src != "/home/anahitik/TUM/Thesis/program-partitioning-for-security-enclaves/test/proto-generator/snake/snake.h") {
            return;
        }
        auto* typedefdecl = decl->getTypedefNameForAnonDecl();
        std::string enumName;
        if (typedefdecl) {
            llvm::dbgs() << "name at typedef " << *typedefdecl << "\n";
            enumName = typedefdecl->getName();
        } else {
            llvm::dbgs() << "Is named here\n";
            enumName = decl->getName();
        }
        ProtoFileGenerator::Enum enumDecl;
        enumDecl.m_name = enumName;
        for (auto it = decl->enumerator_begin(); it != decl->enumerator_end(); ++it) {
            ProtoFileGenerator::Enum::Entry entry = {it->getName(), it->getInitVal().getExtValue()};
            enumDecl.m_entries.push_back(entry);
        }
        m_enums.insert(std::make_pair(enumName, enumDecl));
    }

private:
    Enums m_enums;  
};


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
    vazgen::StructFinder structFinder;
    vazgen::EnumFinder enumFinder;
    //vazgen::TypedefFinder typedefFinder;
    MatchFinder matchFinder;
    matchFinder.addMatcher(vazgen::functionMatcher, &functionFinder);
    matchFinder.addMatcher(vazgen::structMatcher, &structFinder);
    matchFinder.addMatcher(vazgen::enumMatcher, &enumFinder);
    //matchFinder.addMatcher(vazgen::typedefMatcher, &typedefFinder);

    Tool.run(newFrontendActionFactory(&matchFinder).get());


    const auto& functionDecls = functionFinder.getFunctions();
    const auto& structs = structFinder.getStructs();
    const auto& enums = enumFinder.getEnums();
    vazgen::ProtoFileGenerator protoFileGen(functionDecls, structs, enums, "secure_service");
    protoFileGen.generate();

    vazgen::ProtoFileWriter protoWriter("secure_service.proto", protoFileGen.getProtoFile());
    protoWriter.write();

    return 0;
}

