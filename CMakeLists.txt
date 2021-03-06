cmake_minimum_required(VERSION 3.12)
project(program_partitioning VERSION 0.1.0 LANGUAGES CXX)

list(INSERT CMAKE_MODULE_PATH 0 ${CMAKE_SOURCE_DIR}/cmake)
find_package(LLVM 6.0 REQUIRED CONFIG)
find_package (Threads)
find_package(nlohmann_json 3.2.0 REQUIRED)
find_package(svf REQUIRED COMPONENTS Svf)
find_package(pdg REQUIRED)
find_package(Boost COMPONENTS thread REQUIRED)

#TODO: both svf_INCLUDE_DIRS and pdg_INCLUDE_DIRS are empty

# Ideally would use find_package
set (DG_INCLUDE_DIR "/usr/local/include/dg")
set (CPLEX_CONCERT_INCLUDE "/home/anahitik/ibm/concert/include")
set (CPLEX_INCLUDE "/home/anahitik/ibm/cplex/include")
set (CP_INCLUDE "/home/anahitik/ibm/opl/include")
set (CPLEX_CONCERT_LIB "libconcert.a")
set (ILOCPLEX_LIB "libilocplex.a")
set (CPLEX_LIB "libcplex.a")
set (CP_LIB "libcp.a")
# Needed to use dg
add_definitions(-DHAVE_LLVM)
add_definitions(-DENABLE_CFG)
add_definitions(${LLVM_DEFINITIONS})
find_package(Boost REQUIRED)

set(LLVM_LINK_COMPONENTS support)

#message("Boost" ${Boost_INCLUDE_DIRS})
#message("aaa" ${pdg_FOUND})
#message(STATUS "pdg_INCLUDE_DIR:= ${pdg_INCLUDE_DIRS}")
#message(STATUS "pdg_LIBRARY:= ${pdg_LIBRARY}")
#message(STATUS "pdg_LIBRARIES:= ${pdg_LIBRARIES}")

add_executable(sgx-code-gen
                lib/ClangTools/SGXCodeGenerationTool.cpp
                lib/ClangTools/ClangToolUtils.cpp
                lib/CodeGen/SGXCodeGenerator.cpp
                lib/CodeGen/CodeGenUtils.cpp
                lib/CodeGen/Class.cpp
                lib/CodeGen/Function.cpp
                lib/CodeGen/SourceFileWriter.cpp
                lib/CodeGen/FileWriter.cpp
                lib/Utils/Logger.cpp)

target_include_directories(sgx-code-gen PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
        PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
        ${LLVM_INCLUDE_DIRS}
)

#message ("llvm libs" ${LLVM_AVAILABLE_LIBS})
target_link_libraries(sgx-code-gen
                      PRIVATE
                      clangTooling
                      clangDriver
                      clangFrontend
                      clangParse
                      clangSerialization
                      clangSema
                      clangAST
                      clangEdit
                      clangLex
                      clangBasic
                      clangAnalysis
                      #clangCodeGen
                      #clangDynamicASTMatchers
                      clangFormat
                      #clangFrontendTool
                      #clangHandleCXX
                      #clangIndex
                      #clangRewrite
                      #clangRewriteFrontend
                      #clangRewriteFrontend
                      #clangStaticAnalyzerFrontend
                      #clangStaticAnalyzerCheckers
                      #clangStaticAnalyzerCore
                      clangASTMatchers
                      #clangToolingASTDiff
                      clangToolingCore
                      #clangToolingRefactor
                      LLVMSupport
                      LLVMCore
                      LLVMMC
                      LLVMCodeGen
                      LLVMOption
                      #${LLVM_AVAILABLE_LIBS}
)

add_library(program_partitioning MODULE
        lib/Utils/Annotation.cpp
        lib/Utils/ModuleAnnotationParser.cpp
        lib/Utils/JsonAnnotationParser.cpp
        lib/Utils/Logger.cpp
        lib/Utils/Statistics.cpp
        lib/Utils/Utils.cpp
        lib/Utils/PartitionUtils.cpp
        lib/Analysis/ProgramPartitionAnalysis.cpp
        lib/Analysis/PartitionStatistics.cpp
        lib/Analysis/ProgramPartitionStatistics.cpp
        lib/Analysis/Partitioner.cpp
        lib/Analysis/Partition.cpp
        lib/Analysis/CallGraph.cpp
        lib/Optimization/PartitionOptimizer.cpp
        lib/Optimization/PartitionOptimization.cpp
        lib/Optimization/GlobalsMoveToPartitionOptimization.cpp
        lib/Optimization/FunctionsMoveToPartitionOptimization.cpp
        lib/Optimization/DuplicateFunctionsOptimization.cpp
        lib/Optimization/KLOptimizer.cpp
        lib/Optimization/KLOptimizationPass.cpp
        lib/Optimization/ILPOptimization.cpp
        lib/Transforms/PartitionExtractor.cpp
        lib/Transforms/ProtoGeneratorPass.cpp
        lib/CodeGen/FileWriter.cpp
        lib/CodeGen/ProtoFileWriter.cpp
        lib/CodeGen/ProtoFileGenerator.cpp
)

add_library(debug-program_partitioning MODULE
        lib/Debug/PDGPass.cpp
        lib/Debug/AnnotationsTest.cpp
        lib/Debug/ModuleAnnotationsTest.cpp
        lib/Debug/JsonAnnotationsTest.cpp
        lib/Debug/DgPDGCompare.cpp
        #lib/Debug/SVFGTraversal.cpp
        lib/Debug/FunctionExtractor.cpp
        lib/Debug/OptimizationFactorsTable.cpp
        lib/Debug/ILPOptimizationData.cpp
        lib/Eval/AnnotationCreatorPass.cpp
)

add_library(eval-program_partitioning MODULE
        lib/Eval/AnnotationCreatorPass.cpp
        lib/Eval/ShadowCallStackGeneratorPass.cpp
        lib/Eval/ModuleInformation.cpp
        lib/Utils/Utils.cpp
)

target_include_directories(program_partitioning PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
        PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
        ${LLVM_INCLUDE_DIRS}
        ${pdg_INCLUDE_DIR}
        ${CPLEX_CONCERT_INCLUDE}
        ${CPLEX_INCLUDE}
        ${CP_INCLUDE}
)

target_include_directories(debug-program_partitioning PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
        PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
        ${LLVM_INCLUDE_DIRS}
        ${Boost_INCLUDE_DIRS}
        ${DG_INCLUDE_DIR}
        ${pdg_INCLUDE_DIR}
        ${CPLEX_CONCERT_INCLUDE}
        ${CPLEX_INCLUDE}
        ${CP_INCLUDE}
)

target_include_directories(eval-program_partitioning PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
        PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
        ${LLVM_INCLUDE_DIRS}
        ${Boost_INCLUDE_DIRS}
)

target_link_libraries(program_partitioning PRIVATE
                      Threads::Threads
                      svf::Svf
                      nlohmann_json::nlohmann_json
                      ${CPLEX_CONCERT_LIB}
                      ${CP_LIB}
                      ${ILOCPLEX_LIB}
                      ${CPLEX_LIB}
                      m
                      Boost::thread
                      ${CMAKE_DL_LIBS}
)
get_target_property(OUT program_partitioning LINK_LIBRARIES)
message(STATUS ${OUT})

target_link_libraries(debug-program_partitioning PRIVATE
                      Threads::Threads
                      svf::Svf
                      ${Boost_LIBRARIES}
)

target_link_libraries(eval-program_partitioning PRIVATE
                      Threads::Threads
                      ${Boost_LIBRARIES}
)
if ($ENV{CLION_IDE})
    include_directories("/usr/local/include/llvm/")
    include_directories("/usr/local/include/llvm-c/")
endif ()

target_compile_features(program_partitioning PRIVATE cxx_range_for cxx_auto_type cxx_std_17)
target_compile_options(program_partitioning PRIVATE -fno-rtti -g -DIL_STD) #-DNDEBUG)

target_compile_features(debug-program_partitioning PRIVATE cxx_range_for cxx_auto_type cxx_std_17)
target_compile_options(debug-program_partitioning PRIVATE -fno-rtti -g)

target_compile_features(eval-program_partitioning PRIVATE cxx_range_for cxx_auto_type cxx_std_17)
target_compile_options(eval-program_partitioning PRIVATE -fno-rtti -g)

target_compile_features(sgx-code-gen PRIVATE cxx_range_for cxx_auto_type cxx_std_17)
target_compile_options(sgx-code-gen PRIVATE -fno-rtti -g)

