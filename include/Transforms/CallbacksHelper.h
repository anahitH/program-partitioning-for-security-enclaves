#pragma once

#include <memory>
#include <vector>
#include <unordered_map>

#include "llvm/Transforms/Utils/ValueMapper.h"

namespace llvm {
class Argument;
class Function;
class FunctionType;
class Module;
class CallInst;
} // namespace llvm

namespace pdg {
class PDG;
}

namespace vazgen {

class Partition;

class CallbacksHelper
{
public:
    using PDGType = std::shared_ptr<pdg::PDG>;
    using CallbackHandlers = std::unordered_map<llvm::FunctionType*, std::pair<llvm::Function*, llvm::Function*>>;
    using FunctionCallbackArguments = std::unordered_map<llvm::Function*, std::vector<llvm::Argument*>>;

public:
    CallbacksHelper(llvm::Module* module,
                    PDGType pdg,
                    Partition& secure_partition,
                    Partition& insecure_partition);

    CallbacksHelper(const CallbacksHelper& ) = delete;
    CallbacksHelper(CallbacksHelper&& ) = delete;
    CallbacksHelper& operator =(const CallbacksHelper& ) = delete;
    CallbacksHelper& operator =(CallbacksHelper&& ) = delete;

public:
    void adjustCallbacksInPartitions();

private:
    void processFunction(llvm::Function& F);
    llvm::FunctionType* callbackArgumentType(llvm::Argument* argument) const;
    void addCallbackHandlerForFunctionType(llvm::FunctionType* callbackType, llvm::Type* callbackPtrType);
    llvm::Function* createInsecureCallbackHandlerDefinition(llvm::Function* secureCallbackHandler,
                                                            const std::string& uniqueIdStr);
    void modifyFunctionWithCallbackArguments(llvm::Function& F);
    void modifyFunctionWithCallbackArgument(llvm::Function* F, llvm::Argument* arg);
    void replaceIndirectCallWithCallbackArgumentHandler(llvm::Function* F, llvm::Argument* arg, llvm::CallBase* argInvokation);
    void changeFunctionSignature(llvm::Function* F);

private:
    llvm::Module* m_module;
    PDGType m_pdg;
    Partition& m_securePartition;
    Partition& m_insecurePartition;

    FunctionCallbackArguments m_functionCallbackArguments;
    CallbackHandlers m_signatureHandlers;
}; // class CallbacksHelper

} // namespace vazgen

