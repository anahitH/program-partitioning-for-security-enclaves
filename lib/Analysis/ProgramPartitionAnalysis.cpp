#include "Analysis/ProgramPartitionAnalysis.h"

#include "Analysis/Partitioner.h"
#include "Logger.h"
#include "AnnotationParser.h"
#include "JsonAnnotationParser.h"
#include "ModuleAnnotationParser.h"

#include "PDG/Passes/PDGBuildPasses.h"

#include "llvm/Analysis/PostDominators.h"
#include "llvm/IR/Dominators.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Constants.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/raw_ostream.h"

#include "llvm/IR/LegacyPassManager.h"
#include "llvm/PassRegistry.h"
#include "llvm/Transforms/IPO/PassManagerBuilder.h"

#include <fstream>

namespace vazgen {

ProgramPartition::ProgramPartition(llvm::Module& M,
                                   PDGType pdg)
    : m_module(M)
    , m_pdg(pdg)
{
}

void ProgramPartition::partition(const Annotations& annotations)
{
    Partitioner partitioner(m_module, m_pdg);
    m_partition = partitioner.partition(annotations);
}

void ProgramPartition::dump() const
{
    llvm::dbgs() << "Partition of Module " << m_module.getName() << "\n";
    for (auto F : m_partition) {
        llvm::dbgs() << "   " << F->getName() << "\n";
    }
}

llvm::cl::opt<std::string> JsonAnnotations(
    "json-annotations",
    llvm::cl::desc("Json file containing annotations"),
    llvm::cl::value_desc("annotation"));

llvm::cl::opt<std::string> Outfile(
    "outfile",
    llvm::cl::desc("Out file to write partitioning information"),
    llvm::cl::value_desc("outfile name"));


char ProgramPartitionAnalysis::ID = 0;

void ProgramPartitionAnalysis::getAnalysisUsage(llvm::AnalysisUsage& AU) const
{
    AU.addRequired<pdg::SVFGPDGBuilder>();
    AU.setPreservesAll();
}

bool ProgramPartitionAnalysis::runOnModule(llvm::Module& M)
{
    Logger logger("test");
    logger.setLevel(vazgen::Logger::ERR);

    AnnotationParser* annotationParser;
    if (!JsonAnnotations.empty()) {
        annotationParser = new JsonAnnotationParser(&M, JsonAnnotations, logger);
    } else {
        annotationParser = new ModuleAnnotationParser(&M, logger);
    }
    annotationParser->parseAnnotations();
    const auto& annotations = annotationParser->getAllAnnotations();

    auto pdg = getAnalysis<pdg::SVFGPDGBuilder>().getPDG();
    m_partition.reset(new ProgramPartition(M, pdg));
    m_partition->partition(annotations);
    if (!Outfile.empty()) {
        std::ofstream ostr(Outfile);
        for (auto& partitionF : m_partition->getPartition()) {
            ostr << partitionF->getName().str() << "\n";
        }
        ostr.close();
    } else {
        m_partition->dump();
    }

    return false;
}

static llvm::RegisterPass<ProgramPartitionAnalysis> X("partition-analysis","Runs program partitioning analysis");

} // namespace vazgen

