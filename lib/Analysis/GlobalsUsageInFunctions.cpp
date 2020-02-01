#include "Analysis/GlobalsUsageInFunctions.h"

#include "Utils/Logger.h"
#include "Utils/Utils.h"
#include "PDG/PDG/PDG.h"
#include "PDG/PDG/PDGNode.h"
#include "PDG/PDG/PDGEdge.h"
#include "PDG/PDG/FunctionPDG.h"

#include "llvm/IR/Function.h"
#include "llvm/IR/GlobalVariable.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Constants.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/raw_ostream.h"

namespace vazgen {

const GlobalsUsageInFunctions::GlobalVariables& GlobalsUsageInFunctions::getGlobalVariablesUsedInFunction(llvm::Function* F) const
{
    auto it = m_globalsUsedInFunctions.find(F);
    if (it != m_globalsUsedInFunctions.end()) {
        return it->second;
    }
    return GlobalVariables();
}

void GlobalsUsageInFunctions::setGlobalsUsedInFunctions(const GlobalsInFunctions& globalsInFunctions)
{
    m_globalsUsedInFunctions = globalsInFunctions;
}

void GlobalsUsageInFunctions::addGlobalsUsedInFunction(llvm::Function* F, const GlobalVariables& globals)
{
    const auto& [it, result] = m_globalsUsedInFunctions.insert({F, globals});
    if (!result) {
        it->second.insert(it->second.end(), globals.begin(), globals.end());
    }
}

void GlobalsUsageInFunctions::addGlobalUsedInFunction(llvm::Function* F, llvm::GlobalVariable* global)
{
    addGlobalsUsedInFunction(F, {global});
}
 
void GlobalsUsageInFunctions::computeGlobalsUsageInFunctions(llvm::Module& module,
                                                             PDGType pdg,
                                                             Logger& logger)
{
    GlobalsUsageInFunctions& globalsUsage = GlobalsUsageInFunctions::getGlobalsUsageInFunctions();
    logger.info("Analyzing for globals");
    for (auto glob_it = module.global_begin();
         glob_it != module.global_end();
         ++glob_it) {
         assert(pdg->hasGlobalVariableNode(&*glob_it));
         llvm::dbgs() << "analyzing global " << *glob_it << "\n";
         const auto& globalNode = pdg->getGlobalVariableNode(&*glob_it);
         for (auto in_it = globalNode->inEdgesBegin();
              in_it != globalNode->inEdgesEnd();
              ++in_it) {
             llvm::Function* globalUseFunction = Utils::getNodeParent((*in_it)->getSource().get());
             llvm::dbgs() << "in: Global is used in function " << globalUseFunction->getName() << "\n";

             llvm::dbgs() << "adding to partition\n";
             globalsUsage.addGlobalUsedInFunction(globalUseFunction, &*glob_it);
         }
         for (auto out_it = globalNode->outEdgesBegin();
              out_it != globalNode->outEdgesEnd();
              ++out_it) {
             llvm::Function* globalUseFunction = Utils::getNodeParent((*out_it)->getDestination().get());
             llvm::dbgs() << "adding to partition\n";
             globalsUsage.addGlobalUsedInFunction(globalUseFunction, &*glob_it);
         }
    }
}

} // namespace vazgen

