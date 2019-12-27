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
    const auto& functions_with_callback_arg = findFunctionsWithCallbackArgs();
    for (const auto& [function, callbackArgs] : functions_with_callback_arg) {
        createCallbacksHandlers(function, callbackArgs);
        modifyFunctionsWithCallbackArgument(function);
        /* modify function to receive long instead of callback.
            - modify signature
                - most probably by cloning the function to new one and then deleting the original.
                - make sure new function is added to the correct partition
            - modify call of callback to corresponding callback hanlder
            - if callback is passed to another function change that function too
                - might need to start the changes in bottom up direction
          - modify calls of function to pass long instead of callback
            - if is passed an argument of another function, this step will be covered by the previous one
            - otherwise change call site to cast callback to long
        */
    }
}

void CallbacksHelper::createCallbacksHandlers(llvm::Function* F,
        const std::vector<llvm::Argument*>& functionCallbackArgs)
{
    /*
    add callback handler functions for each callback signature type with separate name for both partitions
    e.g. secure_handle_callback_<N>(long address, args...) {
        if (auto* callback = cast<signature_<N>(address)>) {
            callback(args);
        } else {
            insecure_handle_callback(long address, args);
        }
        where insecure_handle_callback is ocall
        add new functions to partitions
    }
    */
    int i = 0;
    for (const auto& arg : functionCallbackArgs) {
        llvm::FunctionType* signatureTy = llvm::dyn_cast<llvm::FunctionType>(arg->getType());
        const auto& callbackHandlers = createCallbackHandlers(signatureTy, i);
        m_signatureHandlers.insert(std::make_pair(signatureTy, callbackHandlers));
        ++i;
    }
}

std::pair<llvm::Function*, llvm::Function*>
CallbacksHelper::createCallbackHandlers(llvm::FunctionType* callbackType, int callbackIdx)
{
    const std::string& secure_handler_name = "secure_callback_handler_" + std::to_string(callbackIdx);
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
        returnValAlloca = builder.CreateAlloca(callbackType->getReturnType(), 8);
    }
    llvm::Instruction* callbackAddrValueAlloca = builder.CreateAlloca(llvm::Type::getInt64Ty(Ctx), 8);
    std::vector<llvm::Instruction*> argumentAllocas;
    argumentAllocas.reserve(callbackType->getNumParams());
    for (int i = 0; i < callbackType->getNumParams(); ++i) {
        argumentAllocas.push_back(builder.CreateAlloca(callbackType->getParamType(i), 8));
    }
    llvm::Instruction* callbackAddress = builder.CreateAlloca(callbackType, 8);

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
    llvm::Value* callbackAddressValueLoad = builder.CreateLoad(callbackAddrValueAlloca);
    llvm::Value* castToCallbackType = builder.CreateIntToPtr(callbackAddressValueLoad, callbackType);
    builder.CreateStore(castToCallbackType, callbackAddress);
    llvm::Instruction* callbackLoad = builder.CreateLoad(callbackAddress);
    llvm::Value* cmpCallback = builder.CreateICmp(llvm::CmpInst::Predicate::ICMP_EQ,
                                                        callbackLoad,
                                                        llvm::Constant::getNullValue(callbackType));

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
    // create insecure clone of this function to insert calls
    llvm::ValueToValueMapTy value_to_value_map;
    llvm::Function* insecureCallbackHandler = createInsecureCloneOfCallbackHandler(secureCallbackHandler, value_to_value_map, callbackIdx);
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
    llvm::IRBuilder<> insecureFunctionBuilder(llvm::dyn_cast<llvm::BasicBlock>(&*value_to_value_map.find(&*ifEndBlock)->second));
    std::vector<llvm::Value*> secureCallbackHandlerCallArgumentLoads;
    secureCallbackHandlerCallArgumentLoads.reserve(callbackType->getNumParams() + 1);
    secureCallbackHandlerCallArgumentLoads.push_back(insecureFunctionBuilder.CreateLoad(&*value_to_value_map.find(callbackAddrValueAlloca)->second));
    for (const auto& argumentAlloca : argumentAllocas) {
        secureCallbackHandlerCallArgumentLoads.push_back(insecureFunctionBuilder.CreateLoad(&*value_to_value_map.find(argumentAlloca)->second));
    }
    llvm::ArrayRef<llvm::Value*> secureCallbackCallArguments(secureCallbackHandlerCallArgumentLoads);
    llvm::Instruction* secureCallbackCall = insecureFunctionBuilder.CreateCall(secureCallbackHandler->getFunctionType(),
                                                secureCallbackHandler,
                                                secureCallbackCallArguments);
    if (callbackType->getReturnType()->isVoidTy()) {
        insecureFunctionBuilder.CreateRetVoid();
    } else {
        insecureFunctionBuilder.CreateStore(secureCallbackCall, &*value_to_value_map.find(returnValAlloca)->second);
        llvm::Instruction* returnValLoad = insecureFunctionBuilder.CreateLoad(&*value_to_value_map.find(returnValAlloca)->second);
        insecureFunctionBuilder.CreateRet(returnValLoad);
    }

    m_securePartition.addToPartition(secureCallbackHandler);
    m_securePartition.addToPartition(insecureCallbackHandler);
    return std::make_pair(secureCallbackHandler, insecureCallbackHandler);
}

llvm::Function* CallbacksHelper::createInsecureCloneOfCallbackHandler(llvm::Function* secureCallbackHandler,
                                                                      llvm::ValueToValueMapTy& valueToValueMap,
                                                                      int callbackIdx)
{
    const std::string& insecure_handler_name = "insecure_callback_handler_" + std::to_string(callbackIdx);
    llvm::Function* insecureCallbackHandler = llvm::CloneFunction(secureCallbackHandler, valueToValueMap);
    insecureCallbackHandler->setName(insecure_handler_name);
    return insecureCallbackHandler;
}

CallbacksHelper::FunctionCallbackArgsMap
CallbacksHelper::findFunctionsWithCallbackArgs()
{
    FunctionCallbackArgsMap functionsWithCallbackArg;
    for (auto& F : *m_module) {
        if (F.isDeclaration() || F.isIntrinsic()) {
            continue;
        }
        for (auto arg_it = F.arg_begin();
             arg_it != F.arg_end();
             ++arg_it) {
            if (arg_it->getType()->isFunctionTy()) {
                functionsWithCallbackArg[&F].push_back(&*arg_it);
                break;
            }
        }
    }
    return functionsWithCallbackArg;
}

void CallbacksHelper::modifyFunctionsWithCallbackArgument(llvm::Function* F)
{
    std::vector<llvm::Type*> callbackArguments(F->getFunctionType()->getNumParams());
    int i = 0;
    std::vector<int> callbackArgIdx;
    callbackArgIdx.reserve(callbackArguments.size());
    for (auto arg_it = F->arg_begin(); arg_it != F->arg_end(); ++arg_it) {
        auto* arg_type = arg_it->getType();
        if (arg_type->isFunctionTy()) {
            callbackArgIdx.push_back(i);
            callbackArguments[i++] = llvm::Type::getInt128Ty(m_module->getContext());
        } else {
            callbackArguments[i++] = arg_it->getType();
        }
    }
    llvm::ArrayRef<llvm::Type*> callbackArgumentsArray(callbackArguments);
    llvm::FunctionType* modifiedFunctionType = llvm::FunctionType::get(F->getReturnType(), callbackArgumentsArray, F->isVarArg());
    llvm::Function* modifiedF = llvm::dyn_cast<llvm::Function>(
                                                    m_module->getOrInsertFunction(F->getName(), modifiedFunctionType).getCallee());
    llvm::ValueToValueMapTy value_to_value_map;
    // map callback args to their address args
    for (const auto& idx : callbackArgIdx) {
        llvm::Argument* callbackArg = F->getArg(idx);
        value_to_value_map[callbackArg] = modifiedF->getArg(idx);
        for (auto user_it = callbackArg->user_begin(); user_it != callbackArg->user_end(); ++user_it) {
            if (llvm::isa<llvm::CallInst>(*user_it)
                    || llvm::isa<llvm::InvokeInst>(*user_it)) {
                //TODO: try to modify in the clone, if doesn't work change in the original function and adjust using value mapper
                //llvm::CallInst* handlerCall = createCallToCallbackHandlerForArg(F, callbackArg);
            }
        }
    }

    llvm::SmallVector<llvm::ReturnInst*, 2> returns;
    llvm::CloneFunctionInto(modifiedF, F, value_to_value_map, false, returns);

    // iterate function instructions, replace callback invokation with corresponding callback handler invokation
}

llvm::CallInst* CallbacksHelper::createCallToCallbackHandlerForArg(llvm::Function* F, llvm::Argument* callbackArg)
{
    llvm::Function* callbackHandler = nullptr;
    llvm::FunctionType* argType = llvm::dyn_cast<llvm::FunctionType>(callbackArg->getType());
    if (m_securePartition.contains(F)) {
        callbackHandler = m_signatureHandlers[argType].first;
    } else {
        callbackHandler = m_signatureHandlers[argType].second;
    }


    // TODO: implement;
    return nullptr;
}

} // namespace vazgen

