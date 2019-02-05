#include "Transforms/ProtoGeneratorPass.h"

#include "Analysis/Partition.h"
#include "Analysis/ProgramPartitionAnalysis.h"
#include "CodeGen/ProtoFileGenerator.h"
#include "CodeGen/ProtoFileWriter.h"

#include "llvm/IR/Module.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/raw_ostream.h"

#include "llvm/IR/LegacyPassManager.h"
#include "llvm/PassRegistry.h"
#include "llvm/Transforms/IPO/PassManagerBuilder.h"

#include <algorithm>
#include <iterator>
#include <list>

namespace vazgen {

char ProtoGeneratorPass::ID = 0;

void ProtoGeneratorPass::getAnalysisUsage(llvm::AnalysisUsage& AU) const
{
    AU.addRequired<ProgramPartitionAnalysis>();
    AU.setPreservesAll();
}

bool ProtoGeneratorPass::runOnModule(llvm::Module& M)
{
    const auto& partition = getAnalysis<ProgramPartitionAnalysis>().getProgramPartition().getSecurePartition();
    ProtoFileGenerator protoGen(partition, M.getName().str());
    protoGen.generate();

    ProtoFileWriter writer(M.getName().str() + "_enclave.proto", protoGen.getProtoFile());
    writer.write();

    return false;
}

static llvm::RegisterPass<ProtoGeneratorPass> X("gen-proto","Generate Proto file for enclave partition");
}

