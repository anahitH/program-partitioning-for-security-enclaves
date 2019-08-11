#include "Transforms/PartitionExtractor.h"

#include "Analysis/Partitioner.h"
#include "Analysis/ProgramPartitionAnalysis.h"
#include "Utils/Utils.h"
#include "Utils/Logger.h"
#include "Utils/Statistics.h"

#include "PDG/Passes/PDGBuildPasses.h"
#include "PDG/PDG/PDG.h"
#include "PDG/PDG/PDGNode.h"
#include "PDG/PDG/PDGEdge.h"
#include "PDG/PDG/FunctionPDG.h"

#include "llvm/IR/Constants.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/ValueMap.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Transforms/Utils/Cloning.h"
#include "llvm/Transforms/Utils/ValueMapper.h"

#include "llvm/IR/LegacyPassManager.h"
#include "llvm/PassRegistry.h"
#include "llvm/Transforms/IPO/PassManagerBuilder.h"

#include <algorithm>
#include <iterator>
#include <list>

namespace vazgen {

namespace {

void addReachableNodes(pdg::PDGNode* node, std::list<pdg::PDGNode*>& list)
{
    for (auto it = node->outEdgesBegin();
         it != node->outEdgesEnd();
         ++it) {
        if ((*it)->isControlEdge()) {
            continue;
        }
         auto* destNode = (*it)->getDestination().get();
         list.push_front(destNode);
    }
}

class GlobalVariableExtractorHelper
{
public:
    GlobalVariableExtractorHelper(llvm::Module* M,
                                  const pdg::PDG& pdg,
                                  const Partition& partition,
                                  const std::string& prefix,
                                  Logger& logger);

    const std::unordered_map<llvm::GlobalVariable*, llvm::Function*>&
    getGlobalVariablesSetters() const
    {
        return m_globalSetter;
    }

    void instrumentForGlobals();

    std::unordered_set<llvm::Function*> getGlobalSetters() const
    {
        std::unordered_set<llvm::Function*> setters;
        for (const auto& [g, f] : m_globalSetter) {
            setters.insert(f);
        }
        return setters;
    }

private:
    void addGlobalSetter(llvm::GlobalVariable* global);
    void addGlobalSetterAfter(llvm::Instruction* instr, llvm::GlobalVariable* global);
    void createGlobalSetterFunction(llvm::GlobalVariable* global);

private:
    llvm::Module* m_module;
    const pdg::PDG& m_pdg;
    const Partition& m_partition;
    std::unordered_map<llvm::GlobalVariable*, llvm::Function*> m_globalSetter;
    const std::string& m_prefix;
    Logger& m_logger;
}; // class GlobalVariableExtractorHelper

GlobalVariableExtractorHelper::GlobalVariableExtractorHelper(llvm::Module* M,
                                                             const pdg::PDG& pdg,
                                                             const Partition& partition,
                                                             const std::string& prefix,
                                                             Logger& logger)
    : m_module(M)
    , m_pdg(pdg)
    , m_partition(partition)
    , m_prefix(prefix)
    , m_logger(logger)
{
}

void GlobalVariableExtractorHelper::instrumentForGlobals()
{
    for (auto* global : m_partition.getGlobals()) {
        addGlobalSetter(global);
    }
}

void GlobalVariableExtractorHelper::addGlobalSetter(llvm::GlobalVariable* global)
{
    if (!m_pdg.hasGlobalVariableNode(global)) {
        m_logger.warn("No pdg node for global " + global->getName().str());
        return;
    }
    auto globalNode = m_pdg.getGlobalVariableNode(global);
    std::list<pdg::PDGNode*> worklist;
    worklist.push_back(globalNode.get());
    while (!worklist.empty()) {
        pdg::PDGNode* currentNode = worklist.back();
        worklist.pop_back();
        auto* llvmNode = llvm::dyn_cast<pdg::PDGLLVMInstructionNode>(currentNode);
        if (!llvmNode) {
            addReachableNodes(currentNode, worklist);
            continue;
        }
        auto* nodeValue = llvmNode->getNodeValue();
        assert(nodeValue);
        if (auto* storeInst = llvm::dyn_cast<llvm::StoreInst>(nodeValue)) {
            if (storeInst->getPointerOperand() == global
                    && m_partition.contains(storeInst->getFunction())) {
                addGlobalSetterAfter(storeInst, global);
            }
        }
        addReachableNodes(currentNode, worklist);
    }
}

void GlobalVariableExtractorHelper::addGlobalSetterAfter(llvm::Instruction* instr,
                                              llvm::GlobalVariable* global)
{
    auto pos = m_globalSetter.find(global);
    if (pos == m_globalSetter.end()) {
        createGlobalSetterFunction(global);
    }
    pos = m_globalSetter.find(global);
    llvm::LLVMContext& Ctx = m_module->getContext();
    llvm::IRBuilder<> builder(instr);
    builder.SetInsertPoint(instr->getParent(), ++builder.GetInsertPoint());
    auto* loadGlobal = builder.CreateLoad(global);
    std::vector<llvm::Value*> arg_values;
    arg_values.push_back(loadGlobal);
    llvm::ArrayRef<llvm::Value*> args(arg_values);
    builder.CreateCall(pos->second, args);
}

// TODO: it's only declaration; add definitions after extraction
void GlobalVariableExtractorHelper::createGlobalSetterFunction(llvm::GlobalVariable* global)
{
    llvm::LLVMContext& Ctx = m_module->getContext();
    llvm::Type* globalType = global->getType();
    if (auto* ptrType = llvm::dyn_cast<llvm::PointerType>(globalType)) {
        globalType = ptrType->getElementType();
    }
    llvm::ArrayRef<llvm::Type*> params{globalType};
    llvm::FunctionType* fType = llvm::FunctionType::get(llvm::Type::getVoidTy(Ctx),
                                                        params, false);
    std::string fName = "set_";
    fName += m_prefix;
    fName += "_";
    fName += global->getName().str();
    llvm::FunctionCallee functionCallee = m_module->getOrInsertFunction(fName, fType);
    llvm::Function* setter = llvm::dyn_cast<llvm::Function>(functionCallee.getCallee());
    m_globalSetter.insert(std::make_pair(global, setter));

    auto* entryBlock = llvm::BasicBlock::Create(Ctx, "entry", setter);
    llvm::IRBuilder<> builder(setter->getContext());
    builder.SetInsertPoint(entryBlock);
    auto* argAlloca = builder.CreateAlloca(globalType, nullptr, "arg");
    auto* storeArg = builder.CreateStore(&*setter->arg_begin(), argAlloca);
    auto* load = builder.CreateLoad(argAlloca);
    auto* storeGlobal = builder.CreateStore(load, global);
    auto* ret = builder.CreateRetVoid();
}

} // unnamed namespace

PartitionExtractor::PartitionExtractor(llvm::Module* M,
                                       const Partition& partition,
                                       const FunctionSet& additionalFunctions,
                                       Logger& logger)
    : m_module(M)
    , m_partition(partition)
    , m_additionalFunctions(additionalFunctions)
    , m_slicedModule(nullptr)
    , m_logger(logger)
{
}

bool PartitionExtractor::extract()
{
    bool modified = false;
    std::unordered_set<std::string> function_names;
    std::vector<std::pair<llvm::Function*, llvm::Function*>> function_mapping;
    for (auto currentF : m_partition.getPartition()) {
        if (!currentF) {
            continue;
        }
        if (currentF->isDeclaration() || currentF->isIntrinsic()) {
            continue;
        }
        //llvm::dbgs() << "extract " << currentF->getName() << "\n";
        //if (currentF->getName() == "main") {
        //    m_logger.warn("Function main in the partition. Can not slice main away");
        //    continue;
        //}
        modified = true;
        function_names.insert(currentF->getName());
    }
    modified |= !m_additionalFunctions.empty();
    for (const auto& F : m_additionalFunctions) {
        function_names.insert(F->getName());
    }
    if (!modified) {
        return modified;
    }
    for (const auto& global : m_partition.getGlobals()) {
        function_names.insert(global->getName());
    }
    createModule(function_names);
    return modified;
}

llvm::Function* PartitionExtractor::createFunctionDeclaration(llvm::Function* originalF)
{
    auto* functionType = originalF->getFunctionType();
    const std::string function_name = originalF->getName();
    const std::string clone_name = function_name + "_clone";
    llvm::FunctionCallee functionCallee = m_module->getOrInsertFunction(clone_name, functionType);
    return llvm::dyn_cast<llvm::Function>(functionCallee.getCallee());
}

bool PartitionExtractor::changeFunctionUses(llvm::Function* originalF, llvm::Function* cloneF)
{
    for (auto it = originalF->user_begin(); it != originalF->user_end(); ++it) {
        if (auto* callInst = llvm::dyn_cast<llvm::CallInst>(*it)) {
            callInst->setCalledFunction(cloneF);
        } else if (auto* invokeInst = llvm::dyn_cast<llvm::InvokeInst>(*it)) {
            invokeInst->setCalledFunction(cloneF);
        } else {
            (*it)->replaceUsesOfWith(originalF, cloneF);
        }
    }
    return true;
}

void PartitionExtractor::createModule(const std::unordered_set<std::string>& function_names)
{
    llvm::ValueToValueMapTy value_to_value_map;
    //llvm::dbgs() << "Clone module for functions\n";
    //for (const auto& f : function_names) {
    //    llvm::dbgs() << f << "\n";
    //}
    m_slicedModule =  llvm::CloneModule(*m_module, value_to_value_map,
                [&function_names] (const llvm::GlobalValue* glob) {
                    return function_names.find(glob->getName()) != function_names.end();
                }
                );
    m_slicedModule->setModuleIdentifier("partition");
    for (auto it = m_slicedModule->begin(); it != m_slicedModule->end(); ++it) {
        if (it->isDeclaration() && it->user_empty()) {
            auto* new_F = &*it;
            ++it;
            new_F->dropAllReferences();
            new_F->eraseFromParent();
        }
    }
    for (auto glob_it = m_slicedModule->global_begin(); glob_it != m_slicedModule->global_end(); ++glob_it) {
        auto* global = &*glob_it;
        if (!global->hasPrivateLinkage() && !global->hasGlobalUnnamedAddr() && global->hasInitializer()) {
            llvm::dbgs() << *global << " has linkage " << global->getLinkage() << "\n";
            global->setLinkage(llvm::GlobalValue::LinkageTypes::InternalLinkage);
        }
    }

}

char PartitionExtractorPass::ID = 0;

void PartitionExtractorPass::getAnalysisUsage(llvm::AnalysisUsage& AU) const
{
    AU.addRequired<ProgramPartitionAnalysis>();
    AU.addRequired<pdg::SVFGPDGBuilder>();
    AU.setPreservesAll();
}

bool PartitionExtractorPass::runOnModule(llvm::Module& M)
{
    Logger logger("program-partitioning");
    logger.setLevel(vazgen::Logger::INFO);

    const auto& enclaveGloabalSetters = getGlobalSetters(M, logger, true);
    const auto& appGloabalSetters = getGlobalSetters(M, logger, false);
    bool modified = extractPartition(logger, M, enclaveGloabalSetters, true);
    modified |= extractPartition(logger, M, appGloabalSetters, false);
    return modified;
}

PartitionExtractorPass::FunctionSet
PartitionExtractorPass::getGlobalSetters(llvm::Module& M, Logger& logger, bool isEnclave)
{
    Partition partition;
    std::string prefixName;
    auto pdg = getAnalysis<pdg::SVFGPDGBuilder>().getPDG();
    if (isEnclave) {
        partition = getAnalysis<ProgramPartitionAnalysis>().getProgramPartition().getInsecurePartition();
        prefixName = "enclave";
    } else {
        partition = getAnalysis<ProgramPartitionAnalysis>().getProgramPartition().getSecurePartition();
        prefixName = "app";
    }
    GlobalVariableExtractorHelper globalsExtractionHelper(&M, *pdg, partition,
                                                          prefixName, logger);

    globalsExtractionHelper.instrumentForGlobals();
    return globalsExtractionHelper.getGlobalSetters();
}

bool PartitionExtractorPass::extractPartition(Logger& logger, llvm::Module& M,
                                              const FunctionSet& globalSetters,
                                              bool enclave)
{
    Partition* partition;
    std::string sliceName;
    auto pdg = getAnalysis<pdg::SVFGPDGBuilder>().getPDG();
    ProgramPartition& programPartition = getAnalysis<ProgramPartitionAnalysis>().getProgramPartition();
    if (enclave) {
        partition = &programPartition.getSecurePartition();
        sliceName = "enclave_lib.bc";
    } else {
        partition = &programPartition.getInsecurePartition();
        sliceName = "app_lib.bc";
    }
    for (const auto& globalSetter : globalSetters) {
        partition->addToPartition(globalSetter);
        partition->addToInInterface(globalSetter);
    }
    m_extractor.reset(new PartitionExtractor(&M, *partition, globalSetters, logger));
    bool modified = m_extractor->extract();
    if (modified) {
        logger.info("Extraction done\n");
        // TODO: this is redundant (and leads to errors) for OpenEnclave framework
        //if (enclave) {
        //    renameInsecureCalls("insecure_", m_extractor->getSlicedModule().get(), programPartition.getInsecurePartition());
        //}
        if (!enclave) {
            if (llvm::Function* mainF = m_extractor->getSlicedModule()->getFunction("main")) {
                mainF->setName("app_main");
            }
        }
        Utils::saveModule(m_extractor->getSlicedModule().get(), sliceName);
    } else {
        logger.info("No extraction\n");
    }
    return modified;
}

void PartitionExtractorPass::renameInsecureCalls(const std::string& prefix, llvm::Module* M, const Partition& insecurePartition)
{
    for (auto& F : *M) {
        if (!F.isDeclaration()) {
            continue;
        }
        if (insecurePartition.containsFunctionWithName(F.getName())) {
            F.setName(prefix + F.getName());
        }
    }
}

static llvm::RegisterPass<PartitionExtractorPass> X("extract-partition","Slice program for a partitioning");

} // namespace vazgen
