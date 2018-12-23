cmake_minimum_required(VERSION 3.12)
project(program_partitioning VERSION 0.1.0 LANGUAGES CXX)

list(INSERT CMAKE_MODULE_PATH 0 ${CMAKE_SOURCE_DIR}/cmake)
find_package(LLVM 6.0 REQUIRED CONFIG)
find_package (Threads)
find_package(nlohmann_json 3.2.0 REQUIRED)
find_package(svf REQUIRED COMPONENTS Svf)
find_package(pdg REQUIRED)

#TODO: both svf_INCLUDE_DIRS and pdg_INCLUDE_DIRS are empty

# Ideally would use find_package
set (DG_INCLUDE_DIR "/usr/local/include/dg")
# Needed to use dg
add_definitions(-DHAVE_LLVM)
add_definitions(-DENABLE_CFG)
find_package(Boost REQUIRED)

#message("Boost" ${Boost_INCLUDE_DIRS})
#message("aaa" ${pdg_FOUND})
#message(STATUS "pdg_INCLUDE_DIR:= ${pdg_INCLUDE_DIRS}")
#message(STATUS "pdg_LIBRARY:= ${pdg_LIBRARY}")
#message(STATUS "pdg_LIBRARIES:= ${pdg_LIBRARIES}")

add_library(program_partitioning MODULE
        lib/Utils/Annotation.cpp
        lib/Utils/ModuleAnnotationParser.cpp
        lib/Utils/JsonAnnotationParser.cpp
        lib/Utils/Logger.cpp
        lib/Utils/Statistics.cpp
        lib/Utils/Utils.cpp
        lib/Utils/PartitionUtils.cpp
        lib/Analysis/ProgramPartitionAnalysis.cpp
        lib/Analysis/Partitioner.cpp
        lib/Analysis/Partition.cpp
        lib/Optimization/PartitionOptimizer.cpp
        lib/Optimization/PartitionOptimization.cpp
        lib/Optimization/GlobalsMoveToPartitionOptimization.cpp
        lib/Optimization/GlobalsMoveOutPartitionOptimization.cpp
        lib/Optimization/FunctionsMoveToPartitionOptimization.cpp
        lib/Optimization/FunctionsMoveOutPartitionOptimization.cpp
        lib/Optimization/DuplicateFunctionsOptimization.cpp
        lib/Transforms/ProgramSlicer.cpp
)

add_library(debug-program_partitioning MODULE
        lib/Debug/PDGPass.cpp
        lib/Debug/AnnotationsTest.cpp
        lib/Debug/ModuleAnnotationsTest.cpp
        lib/Debug/JsonAnnotationsTest.cpp
        lib/Debug/DgPDGCompare.cpp
        #lib/Debug/SVFGTraversal.cpp
        lib/Debug/FunctionExtractor.cpp
)

target_include_directories(program_partitioning PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
        PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
        ${LLVM_INCLUDE_DIRS}
        ${pdg_INCLUDE_DIR}
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
)
target_link_libraries(program_partitioning PRIVATE
                      Threads::Threads
                      svf::Svf
                      nlohmann_json::nlohmann_json
)
target_link_libraries(debug-program_partitioning PRIVATE
                      Threads::Threads
                      svf::Svf
                      ${Boost_LIBRARIES})

if ($ENV{CLION_IDE})
    include_directories("/usr/local/include/llvm/")
    include_directories("/usr/local/include/llvm-c/")
endif ()

target_compile_features(program_partitioning PRIVATE cxx_range_for cxx_auto_type cxx_std_14)
target_compile_options(program_partitioning PRIVATE -fno-rtti -g)

target_compile_features(debug-program_partitioning PRIVATE cxx_range_for cxx_auto_type cxx_std_14)
target_compile_options(debug-program_partitioning PRIVATE -fno-rtti -g)

