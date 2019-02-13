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

const std::string FILE_DELIM = "/";
DeclarationMatcher functionMatcher = functionDecl().bind("functionDecl");
DeclarationMatcher structMatcher = recordDecl().bind("recordDecl");
DeclarationMatcher enumMatcher = enumDecl().bind("enumDecl");
DeclarationMatcher typedefMatcher = typedefDecl().bind("typedefDecl");
static llvm::cl::OptionCategory ProtoFileGenTool("proto-gen options");
static llvm::cl::extrahelp CommonHelp(CommonOptionsParser::HelpMessage);
static llvm::cl::extrahelp MoreHelp("\nMore help text...\n");
static llvm::cl::opt<std::string> functionFile("functions", llvm::cl::cat(ProtoFileGenTool));
static llvm::cl::list<std::string> Files("files", llvm::cl::ZeroOrMore, llvm::cl::cat(ProtoFileGenTool));

class MatcherInFiles
{
public:
    using Files = std::unordered_set<std::string>;

public:
    MatcherInFiles(const Files& files)
        : m_files(files)
    {
    }

protected:
    bool isInFiles(const clang::SourceLocation& loc, const ASTContext* context)
    {
        if (m_files.empty()) {
            return true;
        }
        const SourceManager &srcMgr = context->getSourceManager();
        std::string src = srcMgr.getFilename(loc).str();
        auto pos = src.find_last_of(FILE_DELIM);
        if (pos != std::string::npos) {
            src = src.substr(pos + 1);
        }
        return (m_files.find(src) != m_files.end());
    }

private:
    const Files& m_files;
};

class StructFinder : public MatchFinder::MatchCallback
                   , public MatcherInFiles
{
public:
    using Structs = ProtoFileGenerator::Structs;

public:
    StructFinder(const Files& files)
        : MatcherInFiles(files)
    {
    }

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
        if (!isInFiles(decl->getLocation(), Result.Context)) {
            return;
        }
        if (!decl->isStruct()) {
            return;
        }
        ProtoFileGenerator::Struct decl_struct;
        std::string structName;
        auto* typedefdecl = decl->getTypedefNameForAnonDecl();
        if (typedefdecl) {
            structName = typedefdecl->getName();
        } else {
            structName = decl->getName();
        }
        decl_struct.m_name = structName;
        //llvm::dbgs() << "Found struct " << structName << "\n";
        auto* structDecl = llvm::dyn_cast<RecordDecl>(decl);
        for (auto it = structDecl->field_begin(); it != structDecl->field_end(); ++it) {
            const std::string& fieldName = it->getName().str();
            ProtoFileGenerator::Struct::Field field = {&*it->getType(), fieldName};
            decl_struct.m_fields.push_back(field);
            //llvm::dbgs() << "   Field: " << fieldName << "\n";
        }
        m_structs.insert(std::make_pair(structName, decl_struct));
    }

private:
    Structs m_structs;  
};

class EnumFinder : public MatchFinder::MatchCallback
                 , public MatcherInFiles
{
public:
    using Enums = ProtoFileGenerator::Enums;

public:
    EnumFinder(const Files& files)
        : MatcherInFiles(files)
    {
    }

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
        if (!isInFiles(decl->getLocation(), Result.Context)) {
            return;
        }
        auto* typedefdecl = decl->getTypedefNameForAnonDecl();
        std::string enumName;
        if (typedefdecl) {
            enumName = typedefdecl->getName();
        } else {
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
            if (!decl->isThisDeclarationADefinition()) {
                return;
            }
            m_functions.erase(decl->getNameInfo().getName().getAsString());
            const auto& loc = decl->getLocation();
            m_foundFunctions.insert(decl);
        }
    }

private:
    std::unordered_set<std::string> m_functions;
    std::unordered_set<const FunctionDecl*> m_foundFunctions;
}; // class FunctionFinder


} // namespace vazgen

int main(int argc, const char* argv[])
{
    CommonOptionsParser OptionsParser(argc, argv, vazgen::ProtoFileGenTool);
    vazgen::ProtoFileGenerator protoFileGen;
    protoFileGen.setProtoName("secure_service");
    for (const auto& srcFile : OptionsParser.getSourcePathList()) {
        ClangTool Tool(OptionsParser.getCompilations(),
                       {srcFile});
        const auto& functions = vazgen::parseFunctions(vazgen::functionFile.getValue());
        std::unordered_set<std::string> files;
        for (const auto& F : vazgen::Files) {
            files.insert(F);
        }

        vazgen::FunctionFinder functionFinder(functions);
        vazgen::StructFinder structFinder(files);
        vazgen::EnumFinder enumFinder(files);
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
        protoFileGen.setFunctions(functionDecls);
        protoFileGen.setStructs(structs);
        protoFileGen.setEnums(enums);
        //vazgen::ProtoFileGenerator protoFileGen(functionDecls, structs, enums, "secure_service");
        protoFileGen.generate();
    }

    vazgen::ProtoFileWriter protoWriter("secure_service.proto", protoFileGen.getProtoFile());
    protoWriter.write();

    return 0;
}

