#include "Transforms/CallbacksHelper.h"

#include "Analysis/Partition.h"
#include "Utils/Utils.h"
#include "Utils/Logger.h"

#include "PDG/PDG/PDG.h"
#include "PDG/PDG/PDGNode.h"
#include "PDG/PDG/PDGEdge.h"
#include "PDG/PDG/FunctionPDG.h"

#include "llvm/ADT/APFloat.h"
#include "llvm/ADT/SmallVector.h"
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


namespace vazgen {

namespace {

class UniqueIdGenerator
{
public:
    static UniqueIdGenerator& getUniqueIdGenerator()
    {
        static UniqueIdGenerator uniqueIdStrGenerator;
        return uniqueIdStrGenerator;
    }

    unsigned next()
    {
        return m_id++;
    }

    std::string nextAsString()
    {
        return std::to_string(next());
    }


private:
    UniqueIdGenerator() : m_id(0) {}
      
private:
    unsigned m_id;
};

llvm::CallBase* getCallbackFromNode(llvm::Value* value)
{
    llvm::Instruction* instr = llvm::dyn_cast<llvm::Instruction>(value);
    if (!instr) {
        return nullptr;
    }
    if (auto* callBase = llvm::dyn_cast<llvm::CallBase>(instr)) {
        if (callBase->isIndirectCall()) {
            return callBase;
        }
    }
    return nullptr;
}


} // unnamed namespace


CallbacksHelper::CallbacksHelper(llvm::Module* module,
                                 PDGType pdg,
                                 Partition& secure_partition,
                                 Partition& insecure_partition)
    : m_module(module)
    , m_pdg(pdg)
    , m_securePartition(secure_partition)
    , m_insecurePartition(insecure_partition)
{
}

void CallbacksHelper::adjustCallbacksInPartitions()
{
    for (auto& F : *m_module) {
        if (F.isDeclaration()) {
            continue;
        }
        processFunction(F);
    }
}

void CallbacksHelper::processFunction(llvm::Function& F)
{
    for (auto arg_it = F.arg_begin(); arg_it != F.arg_end(); ++arg_it) {
        llvm::FunctionType* argFtype = callbackArgumentType(&*arg_it);
        if (!argFtype) {
            continue;
        }
        m_functionCallbackArguments[&F].push_back(&*arg_it);
        if (m_signatureHandlers.find(argFtype) == m_signatureHandlers.end()) {
            addCallbackHandlerForFunctionType(argFtype, arg_it->getType());
        }
    }
    if (m_functionCallbackArguments.find(&F) != m_functionCallbackArguments.end()) {
        modifyFunctionWithCallbackArguments(F);
    }
}

llvm::FunctionType* CallbacksHelper::callbackArgumentType(llvm::Argument* argument) const
{
    llvm::Type* arg_type = argument->getType();
    if (arg_type->isFunctionTy()) {
        return llvm::dyn_cast<llvm::FunctionType>(arg_type);
    }
    if (auto* ptr_ty = llvm::dyn_cast<llvm::PointerType>(arg_type)) {
        if (ptr_ty->getElementType()->isFunctionTy()) {
            return llvm::dyn_cast<llvm::FunctionType>(ptr_ty->getElementType());
        }
    }
    return nullptr;
}

void CallbacksHelper::addCallbackHandlerForFunctionType(llvm::FunctionType* callbackType, llvm::Type* callbackPtrType)
{
    const std::string& uniqueIdStr = UniqueIdGenerator::getUniqueIdGenerator().nextAsString();
    const std::string& secure_handler_name = "secure_callback_handler_" + uniqueIdStr;
    llvm::LLVMContext& Ctx = m_module->getContext();
    std::vector<llvm::Type*> callbackHandlerParamsTypes(1 + callbackType->getNumParams());
    callbackHandlerParamsTypes[0] = llvm::Type::getInt64Ty(Ctx);
    for (int i = 1; i < callbackHandlerParamsTypes.size(); ++i) {
        callbackHandlerParamsTypes[i] = callbackType->getParamType(i - 1);
    }
    llvm::ArrayRef<llvm::Type*> callbackHandlerParams(callbackHandlerParamsTypes);
    llvm::FunctionType* callbackHanlderType = llvm::FunctionType::get(callbackType->getReturnType(),
                                                                      callbackHandlerParams, false);
    llvm::Function* secureCallbackHandler = llvm::dyn_cast<llvm::Function>(
                                                    m_module->getOrInsertFunction(secure_handler_name, callbackHanlderType).getCallee());
    // Add the body
    llvm::BasicBlock* entryBlock = llvm::BasicBlock::Create(Ctx, "entry", secureCallbackHandler);
    llvm::IRBuilder<> builder(entryBlock);
    // allocations
    llvm::Instruction* returnValAlloca = nullptr;
    if (!callbackType->getReturnType()->isVoidTy()) {
        returnValAlloca = builder.CreateAlloca(callbackType->getReturnType()); //, 8);
    }
    llvm::Instruction* callbackAddrValueAlloca = builder.CreateAlloca(llvm::Type::getInt64Ty(Ctx)); //, 8);
    std::vector<llvm::Instruction*> argumentAllocas;
    argumentAllocas.reserve(callbackType->getNumParams());
    for (int i = 0; i < callbackType->getNumParams(); ++i) {
        argumentAllocas.push_back(builder.CreateAlloca(callbackType->getParamType(i))); //, 8));
    }
    llvm::Instruction* callbackAddress = builder.CreateAlloca(callbackPtrType); //, 8);

    // store arguments
    // store the first argument - callback address value
    builder.CreateStore(&*secureCallbackHandler->arg_begin(), callbackAddrValueAlloca);
    // store the rest of arguments
    int arg_idx = 0;
    for (auto arg_it = secureCallbackHandler->arg_begin() + 1;
         arg_it != secureCallbackHandler->arg_end(); ++arg_it) {
        builder.CreateStore(&*arg_it, argumentAllocas[arg_idx++]);
    }

    // cast to callback type and check
    llvm::Value* callbackAddressValueLoad = builder.CreateLoad(/*llvm::Type::getInt64PtrTy(Ctx), */callbackAddrValueAlloca);
    llvm::dbgs() << *callbackType << "\n";
    llvm::Value* castToCallbackType = builder.CreateIntToPtr(callbackAddressValueLoad, callbackPtrType);
    builder.CreateStore(castToCallbackType, callbackAddress);
    llvm::Instruction* callbackLoad = builder.CreateLoad(callbackAddress);
    llvm::Value* cmpCallback = builder.CreateICmp(llvm::CmpInst::Predicate::ICMP_EQ,
                                                        callbackLoad,
                                                        llvm::Constant::getNullValue(callbackPtrType));

    llvm::BasicBlock* ifThenBlock = llvm::BasicBlock::Create(Ctx, "if.then", secureCallbackHandler);
    llvm::BasicBlock* ifEndBlock = llvm::BasicBlock::Create(Ctx, "if.end", secureCallbackHandler);
    builder.CreateCondBr(cmpCallback, ifThenBlock, ifEndBlock);

    // create ifThenBlock
    builder.SetInsertPoint(ifThenBlock);
    llvm::Instruction* loadCallbackToCall = builder.CreateLoad(callbackAddress);
    // load Arguments
    std::vector<llvm::Value*> callArgumentLoads;
    callArgumentLoads.reserve(callbackType->getNumParams());
    for (const auto& argumentAlloca : argumentAllocas) {
        callArgumentLoads.push_back(builder.CreateLoad(argumentAlloca));
    }
    llvm::ArrayRef<llvm::Value*> callArguments(callArgumentLoads);
    llvm::Instruction* call = builder.CreateCall(callbackType, loadCallbackToCall, callArguments);
    if (!callbackType->getReturnType()->isVoidTy()) {
        builder.CreateStore(call, returnValAlloca);
    }
    builder.CreateBr(ifEndBlock);

    // create ifEndBlock
    // 1. create insecure clone of this function to insert calls
    // first create the definition of the insecure callbackhandler. finish with the creation of the secure handler, then do the clone
    llvm::Function* insecureCallbackHandler = createInsecureCallbackHandlerDefinition(secureCallbackHandler, uniqueIdStr);
    builder.SetInsertPoint(ifEndBlock);

    std::vector<llvm::Value*> insecureCallbackHandlerCallArgumentLoads;
    insecureCallbackHandlerCallArgumentLoads.reserve(callbackType->getNumParams() + 1);
    insecureCallbackHandlerCallArgumentLoads.push_back(builder.CreateLoad(callbackAddrValueAlloca));
    for (const auto& argumentAlloca : argumentAllocas) {
        insecureCallbackHandlerCallArgumentLoads.push_back(builder.CreateLoad(argumentAlloca));
    }
    llvm::ArrayRef<llvm::Value*> insecureCallbackCallArguments(insecureCallbackHandlerCallArgumentLoads);
    llvm::Instruction* insecureCallbackCall = builder.CreateCall(insecureCallbackHandler->getFunctionType(),
                                                insecureCallbackHandler,
                                                insecureCallbackCallArguments);
    if (callbackType->getReturnType()->isVoidTy()) {
        builder.CreateRetVoid();
    } else {
        builder.CreateStore(insecureCallbackCall, returnValAlloca);
        llvm::Instruction* returnValLoad = builder.CreateLoad(returnValAlloca);
        builder.CreateRet(returnValLoad);
    }

    // finish insecure callback handler
    //llvm::ValueToValueMapTy value_to_value_map;
//    llvm::IRBuilder<> insecureFunctionBuilder(llvm::dyn_cast<llvm::BasicBlock>(&*value_to_value_map.find(&*ifEndBlock)->second));
//    std::vector<llvm::Value*> secureCallbackHandlerCallArgumentLoads;
//    secureCallbackHandlerCallArgumentLoads.reserve(callbackType->getNumParams() + 1);
//    secureCallbackHandlerCallArgumentLoads.push_back(insecureFunctionBuilder.CreateLoad(&*value_to_value_map.find(callbackAddrValueAlloca)->second));
//    for (const auto& argumentAlloca : argumentAllocas) {
//        secureCallbackHandlerCallArgumentLoads.push_back(insecureFunctionBuilder.CreateLoad(&*value_to_value_map.find(argumentAlloca)->second));
//    }
//    llvm::ArrayRef<llvm::Value*> secureCallbackCallArguments(secureCallbackHandlerCallArgumentLoads);
//    llvm::Instruction* secureCallbackCall = insecureFunctionBuilder.CreateCall(secureCallbackHandler->getFunctionType(),
//                                                secureCallbackHandler,
//                                                secureCallbackCallArguments);
//    if (callbackType->getReturnType()->isVoidTy()) {
//        insecureFunctionBuilder.CreateRetVoid();
//    } else {
//        insecureFunctionBuilder.CreateStore(secureCallbackCall, &*value_to_value_map.find(returnValAlloca)->second);
//        llvm::Instruction* returnValLoad = insecureFunctionBuilder.CreateLoad(&*value_to_value_map.find(returnValAlloca)->second);
//        insecureFunctionBuilder.CreateRet(returnValLoad);
//    }
//
    m_securePartition.addToPartition(secureCallbackHandler);
    m_securePartition.addToPartition(insecureCallbackHandler);

    m_signatureHandlers.insert(std::make_pair(callbackType, std::make_pair(secureCallbackHandler, insecureCallbackHandler)));
}

llvm::Function* CallbacksHelper::createInsecureCallbackHandlerDefinition(llvm::Function* secureCallbackHandler,
                                                                      const std::string& uniqueIdStr)
{
    const std::string& insecure_handler_name = "insecure_callback_handler_" + uniqueIdStr;
    llvm::Function* insecureCallbackHandler = llvm::dyn_cast<llvm::Function>(
                                                    m_module->getOrInsertFunction(insecure_handler_name, secureCallbackHandler->getFunctionType()).getCallee());
    return insecureCallbackHandler;
}

void CallbacksHelper::modifyFunctionWithCallbackArguments(llvm::Function& F)
{
    //Thoughts: can not modify then change function signature, as during the modify inserts function call which calready needs a long instead of function type
    // try this:
    // modify function. in the call of callbackhandler pass a placeholder (e.g. 0) instead of the long address. cache the arg index for each placeholder, e.g. i -> j means arg i-th should go to the j-th placeholder
    // change function signature, clone previously modfied function to the new one
    // find all callbackhandler calls in the changed function and make the arg replacements accoring to the saved cache
    // how about when callback argument is assigned a value in this function??
    const auto& callbackArguments = m_functionCallbackArguments[&F];
    assert(!callbackArguments.empty());
    for (auto* arg : callbackArguments) {
        modifyFunctionWithCallbackArgument(&F, arg);
    }
    changeFunctionSignature(&F);
}

void CallbacksHelper::modifyFunctionWithCallbackArgument(llvm::Function* F, llvm::Argument* arg)
{
    auto Fpdg = m_pdg->getFunctionPDG(F);
    auto formalArgNode = Fpdg->getFormalArgNode(arg);
    std::list<pdg::FunctionPDG::PDGNodeTy> workingList;
    std::unordered_set<llvm::Value*> processed_values;
    workingList.push_back(formalArgNode);
    while (!workingList.empty()) {
        pdg::PDGNode* currentNode = workingList.back().get();
        workingList.pop_back();
        auto* llvmNode = llvm::dyn_cast<pdg::PDGLLVMNode>(currentNode);
        if (!llvmNode) {
            continue;
        }
        auto* nodeValue = llvmNode->getNodeValue();
        if (!nodeValue) {
            continue;
        }
        llvm::dbgs() << *nodeValue << "\n";
        if (!processed_values.insert(nodeValue).second) {
            continue;
        }
        if (llvm::CallBase* indirectCall = getCallbackFromNode(nodeValue)) {
            // TODO: check if this is enough to say that this is invokation of the arg
            if (indirectCall->getFunctionType() != callbackArgumentType(arg)) {
                continue;
            }
            replaceIndirectCallWithCallbackArgumentHandler(F, arg, indirectCall);
        }

        for (auto out_it = currentNode->outEdgesBegin();
                out_it != currentNode->outEdgesEnd();
                ++out_it) {
            auto destNode = (*out_it)->getDestination();
            workingList.push_front(destNode);
        }
    }
}

void CallbacksHelper::replaceIndirectCallWithCallbackArgumentHandler(llvm::Function* F, llvm::Argument* arg, llvm::CallBase* argInvokation)
{
    // TODO: consider caching the arg -> callback type, as we are calling callbackArgumentType function quite often
    const auto& callbackHandlers = m_signatureHandlers.find(callbackArgumentType(arg));
    assert(callbackHandlers != m_signatureHandlers.end());
    llvm::Function* callbackHandler = m_securePartition.contains(F) ? callbackHandlers->second.first : callbackHandlers->second.second;
    // TODO: debug and check if this will work. Otherwise may need to create a new callInst
    // Can not do this. need to insert a new call
     std::vector<llvm::Value*> callArguments;
    callArguments.reserve(argInvokation->getFunctionType()->getNumParams() + 1);
    llvm::IRBuilder<> builder(argInvokation);
    callArguments.push_back(builder.CreatePtrToInt(arg, llvm::Type::getInt64Ty(F->getContext())));
    for (int i = 0; i < argInvokation->getNumArgOperands(); ++i) {
        callArguments.push_back(argInvokation->getArgOperand(i));
    }
    llvm::ArrayRef<llvm::Value*> callArgumentsArray(callArguments);
    llvm::Instruction* callbackHandlerCall = builder.CreateCall(callbackHandler->getFunctionType(), callbackHandler, callArgumentsArray);

    for (auto user_it = argInvokation->user_begin(); user_it != argInvokation->user_end(); ++user_it) {
        user_it->replaceUsesOfWith(argInvokation, callbackHandlerCall);
    }
    argInvokation->eraseFromParent();
}

void CallbacksHelper::changeFunctionSignature(llvm::Function* F)
{
    llvm::LLVMContext& Ctx = m_module->getContext();
    const auto& callbackArguments = m_functionCallbackArguments[F];

    llvm::FunctionType* Ftype = F->getFunctionType();
    std::vector<llvm::Type*> modifiedFArguments(Ftype->getNumParams());
    for (llvm::Argument* arg : callbackArguments) {
        modifiedFArguments[arg->getArgNo()] = llvm::Type::getInt64Ty(Ctx);
    }
    for (int i = 0; i < Ftype->getNumParams(); ++i) {
        if (modifiedFArguments[i] == nullptr) {
            modifiedFArguments[i] = Ftype->getParamType(i);
        }
    }
    llvm::ArrayRef<llvm::Type*> modifiedFParams(modifiedFArguments);
    llvm::FunctionType* modifiedFType = llvm::FunctionType::get(F->getReturnType(),
                                                                modifiedFParams, false);
    const std::string modifiedFName = F->getName().str() + "_modified";
    llvm::Function* modifiedF = llvm::dyn_cast<llvm::Function>(
                                                    m_module->getOrInsertFunction(modifiedFName, modifiedFType).getCallee());
    llvm::ValueToValueMapTy value_to_value_map;
    for (auto f_arg_it = F->arg_begin(), modified_f_arg_it = modifiedF->arg_begin();
            f_arg_it != F->arg_end(); ++f_arg_it, ++modified_f_arg_it) {
        value_to_value_map[&*f_arg_it] = &*modified_f_arg_it;
    }
    llvm::SmallVector<llvm::ReturnInst*, 2> returns;
    llvm::CloneFunctionInto(modifiedF, F, value_to_value_map, false, returns);

    // change the usages of F with modifiedF
    auto functionNode = m_pdg->getFunctionNode(F);
    for (auto in_edge = functionNode->inEdgesBegin(); in_edge != functionNode->inEdgesEnd(); ++in_edge) {
        if (!(*in_edge)->isControlEdge()) {
            continue;
        }
        pdg::PDG::PDGNodeTy sourceNode = (*in_edge)->getSource();
        auto* llvmNode = llvm::dyn_cast<pdg::PDGLLVMNode>(sourceNode.get());
        if (!llvmNode) {
            llvm::dbgs() << "Unable to retrieve llvm node for function use " << F->getName() << "\n";
            continue;
        }
        llvm::Value* value = llvmNode->getNodeValue();
        if (auto* callBase = llvm::dyn_cast<llvm::CallBase>(value)) {
            callBase->setCalledFunction(modifiedF);
        }
    }

    // remove old function
    const std::string Fname = F->getName();
    F->eraseFromParent();
    modifiedF->setName(Fname);
}

} // namespace vazgen

