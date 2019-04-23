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
} // namespace llvm

namespace pdg {
class PDG;
}

namespace vazgen {

class Partition;

// TODO: rename the class
class CallbacksHelper
{
public:
    using PDGType = std::shared_ptr<pdg::PDG>;
    using FunctionCallbackArgsMap = std::unordered_map<llvm::Function*, std::vector<llvm::Argument*>>;
    using CallbackHandlers = std::unordered_map<llvm::FunctionType*, std::pair<llvm::Function*, llvm::Function*>>;

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
    FunctionCallbackArgsMap findFunctionsWithCallbackArgs();
    void addCallbackHandlerFunctions(const FunctionCallbackArgsMap& functionsWithCallbackArgs);
    void createCallbacksHanlders(llvm::Function* F,
                                 const std::vector<llvm::Argument*>& functionWithCallbackArgs);
    std::pair<llvm::Function*, llvm::Function*> createCallbackHandlers(llvm::FunctionType* callbackType, int callbackIdx);
    llvm::Function* createInsecureCloneOfCallbackHandler(llvm::Function* secureCallbackHandler,
                                                         llvm::ValueToValueMapTy& valueToValueMap,
                                                         int callbackIdx);

private:
    llvm::Module* m_module;
    PDGType m_pdg;
    Partition& m_securePartition;
    Partition& m_insecurePartition;
    CallbackHandlers m_signatureHandlers;
}; // class CallbacksHelper

} // namespace vazgen

