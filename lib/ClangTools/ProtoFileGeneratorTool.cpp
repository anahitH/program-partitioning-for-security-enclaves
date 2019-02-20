#include "clang/ASTMatchers/ASTMatchers.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"
#include "clang/Frontend/FrontendActions.h"
#include "clang/Tooling/CommonOptionsParser.h"
#include "clang/Tooling/Tooling.h"
#include "llvm/Support/CommandLine.h"

#include "ClangTools/ProtoFileGenerator.h"
#include "CodeGen/ProtoFileWriter.h"
#include "Utils/Logger.h"

#include "nlohmann/json.hpp"

#include <fstream>
#include <iostream>
#include <unordered_set>
#include <unordered_map>

using namespace clang;
using namespace clang::ast_matchers;
using namespace clang::tooling;

namespace vazgen {

using Functions =  std::unordered_set<std::string>;

Functions parseFunctions(const std::string& fileName)
{
    std::unordered_set<std::string> functions;
    std::ifstream fileStream(fileName);
    std::string function;
    while (std::getline(fileStream, function)) {
        functions.insert(function);
    }
    return functions;
}

std::pair<Functions, Functions> parseFunctionsFromStats(const std::string& statsFile, Logger& logger)
{
    using namespace nlohmann;
    std::pair<Functions, Functions> result;
    std::ifstream ifs (statsFile, std::ifstream::in);
    if (!ifs.is_open()) {
        logger.error("Could not open annotations' json file " + statsFile + "\n");
        return result;
    }
    json stats;
    ifs >> stats;
    const auto& secureFunctions = stats["partition"]["secure_partition"]["in_interface"];
    result.first.insert(secureFunctions.begin(), secureFunctions.end());
    const auto& insecureFunctions = stats["partition"]["insecure_partition"]["in_interface"];
    result.second.insert(insecureFunctions.begin(), insecureFunctions.end());
}

const std::string FILE_DELIM = "/";
DeclarationMatcher functionMatcher = functionDecl().bind("functionDecl");
DeclarationMatcher structMatcher = recordDecl().bind("recordDecl");
DeclarationMatcher enumMatcher = enumDecl().bind("enumDecl");
DeclarationMatcher typedefMatcher = typedefDecl().bind("typedefDecl");

static llvm::cl::OptionCategory ProtoFileGenTool("proto-gen options");
static llvm::cl::extrahelp CommonHelp(CommonOptionsParser::HelpMessage);
static llvm::cl::extrahelp MoreHelp("\nMore help text...\n");
static llvm::cl::opt<std::string> functionStats("function-stats",
                                                llvm::cl::desc("Statistics json file containing functions to create proto file for."),
                                                llvm::cl::ZeroOrMore, llvm::cl::cat(ProtoFileGenTool));
static llvm::cl::opt<std::string> functionFile("functions",
                                               llvm::cl::desc("Text file containing functions to create proto file for."),
                                               llvm::cl::ZeroOrMore, llvm::cl::cat(ProtoFileGenTool));
static llvm::cl::opt<std::string> protoName("proto-name",
                                            llvm::cl::desc("Name for proto file and the service"),
                                            llvm::cl::ZeroOrMore, llvm::cl::cat(ProtoFileGenTool));

// TODO: remove
class MatcherInFiles
{
public:
    using Files = std::unordered_set<std::string>;

public:
    MatcherInFiles() = default;
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
{
public:
    using Structs = ProtoFileGenerator::Structs;

public:
    StructFinder() = default;

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
{
public:
    using Enums = ProtoFileGenerator::Enums;

public:
    EnumFinder() = default;

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

    std::unordered_set<const FunctionDecl*> getFunctions()
    {
        std::unordered_set<const FunctionDecl*> functions;
        std::transform(m_functionDecls.begin(), m_functionDecls.end(), std::inserter(functions, functions.begin()),
            [] (const auto& item ) {return item.second; });
        return functions;
    }

public:
    virtual void run(const MatchFinder::MatchResult &Result) override
    {
        const FunctionDecl* decl = Result.Nodes.getNodeAs<clang::FunctionDecl>("functionDecl");
        if (!decl) {
            return;
        }
        if (m_functions.find(decl->getNameInfo().getName().getAsString()) != m_functions.end()) {
            //if (!decl->isDefined()) {
            //    llvm::dbgs() << decl->getName() << " is not defined\n";
            //    const SourceManager &srcMgr = Result.Context->getSourceManager();
            //    std::string src = srcMgr.getFilename(decl->getLocation()).str();
            //    llvm::dbgs() << src << "\n";
            //    decl->dump();
            //}
            // TODO: test this with memcached
            if (decl->isDefined() && !decl->isThisDeclarationADefinition()) {
                return;
            }
            if (decl->isDefined()) {
                m_functions.erase(decl->getNameInfo().getName().getAsString());
            } else {
                //auto pos = m_functionDecls.find(decl->getName());
                //if (pos != m_functionDecls.end()) {
                //    m_foundFunctions.erase(pos->second);
                //}
            }
            m_functionDecls[decl->getName()] = decl;
            const auto& loc = decl->getLocation();
            //m_foundFunctions.insert(decl);
        }
    }

private:
    std::unordered_set<std::string> m_functions;
    std::unordered_set<const FunctionDecl*> m_foundFunctions;
    std::unordered_map<std::string, const FunctionDecl*> m_functionDecls;
}; // class FunctionFinder


} // namespace vazgen

void run(CommonOptionsParser& OptionsParser,
         const std::unordered_set<std::string>& functions,
         const std::string& protoName)
{
    vazgen::ProtoFileGenerator protoFileGen;
    protoFileGen.setProtoName(protoName + "_service");

    for (const auto& srcFile : OptionsParser.getSourcePathList()) {
        ClangTool Tool(OptionsParser.getCompilations(), {srcFile});
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
        protoFileGen.setFunctions(functionDecls);
        protoFileGen.setStructs(structs);
        protoFileGen.setEnums(enums);
        protoFileGen.generate();
    }

    vazgen::ProtoFileWriter protoWriter(protoName + "_service.proto", protoFileGen.getProtoFile());
    protoWriter.write();
}

int main(int argc, const char* argv[])
{
    CommonOptionsParser OptionsParser(argc, argv, vazgen::ProtoFileGenTool);
    vazgen::Logger logger("clang-tool");
    logger.setLevel(vazgen::Logger::ERR);
    
    if (!vazgen::functionFile.empty()) {
        const auto& functions = vazgen::parseFunctions(vazgen::functionFile.getValue());
        run(OptionsParser, functions, vazgen::protoName.empty() ? "partition_proto" : vazgen::protoName.getValue());
    } else if (!vazgen::functionStats.empty()) {
        const auto& [secureFunctions, insecureFunctions] = vazgen::parseFunctionsFromStats(vazgen::functionStats.getValue(), logger);
        run(OptionsParser, secureFunctions, "secure_enclave");
        run(OptionsParser, insecureFunctions, "insecure_app");
    } else {
        logger.error("No file is specified for functions");
        return 0;
    }
    return 0;
}

