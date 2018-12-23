#include "Analysis/ProgramPartitionAnalysis.h"

#include "Analysis/Partitioner.h"
#include "Utils/Logger.h"
#include "Utils/Statistics.h"
#include "Utils/AnnotationParser.h"
#include "Utils/JsonAnnotationParser.h"
#include "Utils/ModuleAnnotationParser.h"
#include "Optimization/PartitionOptimizer.h"

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

#include <algorithm>
#include <fstream>

namespace vazgen {

class ProgramPartition::PartitionStatistics : public Statistics
{
public:
    PartitionStatistics(std::ofstream& strm,
                        const Partition& partition,
                        llvm::Module& M);

    void report() final;

private:
    const Partition& m_partition;
    llvm::Module& m_module;
}; // class PartitionStatistics

ProgramPartition::PartitionStatistics::PartitionStatistics(std::ofstream& strm,
                                                           const Partition& partition,
                                                           llvm::Module& M)
    : Statistics(strm, Statistics::JSON)
    , m_partition(partition)
    , m_module(M)
{
}

void ProgramPartition::PartitionStatistics::report()
{
    std::vector<std::string> partitionFs;
    partitionFs.reserve(m_partition.getPartition().size());
    std::transform(m_partition.getPartition().begin(), m_partition.getPartition().end(), std::back_inserter(partitionFs),
            [] (llvm::Function* F) { return F->getName().str();});
    write_entry({"program_partition", "partitioned_functions"}, partitionFs);
    write_entry({"program_partition", "partition_size"}, (unsigned) partitionFs.size());
    double partition_portion = (m_partition.getPartition().size() * 100.0) / m_module.size();
    write_entry({"program_partition", "partition%"}, partition_portion);

    std::vector<std::string> inInterface;
    inInterface.reserve(m_partition.getInInterface().size());
    std::transform(m_partition.getInInterface().begin(), m_partition.getInInterface().end(), std::back_inserter(inInterface),
            [] (llvm::Function* F) { return F->getName().str();});
    write_entry({"program_partition", "in_interface"}, inInterface);
    write_entry({"program_partition", "in_interface_size"}, (unsigned) inInterface.size());
    double inInterface_portion = (m_partition.getInInterface().size() * 100.0) / m_module.size();
    write_entry({"program_partition", "in_interface%"}, inInterface_portion);

    std::vector<std::string> outInterface;
    inInterface.reserve(m_partition.getOutInterface().size());
    std::transform(m_partition.getOutInterface().begin(), m_partition.getOutInterface().end(), std::back_inserter(outInterface),
            [] (llvm::Function* F) { return F->getName().str();});
    write_entry({"program_partition", "out_interface"}, outInterface);
    write_entry({"program_partition", "out_interface_size"}, (unsigned) outInterface.size());
    double outInterface_portion = (m_partition.getOutInterface().size() * 100.0) / m_module.size();
    write_entry({"program_partition", "out_interface%"}, outInterface_portion);

    flush();
}

ProgramPartition::ProgramPartition(llvm::Module& M,
                                   PDGType pdg,
                                   Logger& logger)
    : m_module(M)
    , m_pdg(pdg)
    , m_logger(logger)
{
}

void ProgramPartition::partition(const Annotations& annotations)
{
    Partitioner partitioner(m_module, m_pdg, m_logger);
    partitioner.partition(annotations);
    m_securePartition = partitioner.getSecurePartition();
    m_insecurePartition = partitioner.getInsecurePartition();
}

void ProgramPartition::optimize()
{
    PartitionOptimizer optimizer(m_securePartition, m_insecurePartition, m_logger);
    optimizer.run();
}

void ProgramPartition::dump(const std::string& outFile) const
{
    const auto& partitionFs = m_securePartition.getPartition();
    if (!outFile.empty()) {
        std::ofstream ostr(outFile);
        for (auto& partitionF : partitionFs) {
            ostr << partitionF->getName().str() << "\n";
        }
        ostr.close();
        return;
    }
    llvm::dbgs() << "Partition of Module " << m_module.getName() << "\n";
    for (auto F : partitionFs) {
        llvm::dbgs() << "   " << F->getName() << "\n";
    }
}

void ProgramPartition::dumpStats(const std::string& statsFile) const
{
    std::ofstream strm;
    if (statsFile.empty()) {
        strm.open("partition_stats.json");
    } else {
        strm.open(statsFile);
    }
    PartitionStatistics stats(strm, m_securePartition, m_module);
    stats.report();
}

llvm::cl::opt<std::string> JsonAnnotations(
    "json-annotations",
    llvm::cl::desc("Json file containing annotations"),
    llvm::cl::value_desc("annotation"));

llvm::cl::opt<std::string> Outfile(
    "outfile",
    llvm::cl::desc("Out file to write partitioning information"),
    llvm::cl::value_desc("outfile name"));

llvm::cl::opt<bool> Stats(
    "partition-stats",
    llvm::cl::desc("Dump partition stats"),
    llvm::cl::value_desc("flag to dump stats"));

// This is running the simplest optimization passes
llvm::cl::opt<bool> Opt(
    "optimize",
    llvm::cl::desc("Optimize partition"),
    llvm::cl::value_desc("boolean flag"));

char ProgramPartitionAnalysis::ID = 0;

void ProgramPartitionAnalysis::getAnalysisUsage(llvm::AnalysisUsage& AU) const
{
    AU.addRequired<pdg::SVFGPDGBuilder>();
    AU.setPreservesAll();
}

bool ProgramPartitionAnalysis::runOnModule(llvm::Module& M)
{
    Logger logger("program-partitioning");
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
    m_partition.reset(new ProgramPartition(M, pdg, logger));
    m_partition->partition(annotations);
    if (Opt) {
        m_partition->optimize();
    }
    m_partition->dump(Outfile);
    if (Stats) {
        m_partition->dumpStats();
    }

    return false;
}

static llvm::RegisterPass<ProgramPartitionAnalysis> X("partition-analysis","Runs program partitioning analysis");

} // namespace vazgen

