cmake_minimum_required(VERSION 3.12)
project(program_partitioning VERSION 0.1.0 LANGUAGES CXX)

list(INSERT CMAKE_MODULE_PATH 0 ${CMAKE_SOURCE_DIR}/cmake)
find_package(LLVM 6.0 REQUIRED CONFIG)
find_package (Threads)
find_package(nlohmann_json 3.2.0 REQUIRED)
find_package(svf REQUIRED COMPONENTS Svf)

add_library(program_partitioning MODULE
        lib/ModuleAnnotationParser.cpp
        lib/JsonAnnotationParser.cpp
        lib/Logger.cpp
)

add_library(debug-program_partitioning MODULE
        lib/Debug/PDGPass.cpp
        lib/Debug/AnnotationsTest.cpp
        lib/Debug/ModuleAnnotationsTest.cpp
        lib/Debug/JsonAnnotationsTest.cpp
)

target_include_directories(program_partitioning PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
        PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
        ${LLVM_INCLUDE_DIRS}
)

target_include_directories(debug-program_partitioning PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
        PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
        ${LLVM_INCLUDE_DIRS}
)
target_link_libraries(program_partitioning PRIVATE
                      Threads::Threads
                      svf::Svf
                      nlohmann_json::nlohmann_json
)
target_link_libraries(debug-program_partitioning PRIVATE Threads::Threads svf::Svf)

if ($ENV{CLION_IDE})
    include_directories("/usr/local/include/llvm/")
    include_directories("/usr/local/include/llvm-c/")
endif ()

target_compile_features(program_partitioning PRIVATE cxx_range_for cxx_auto_type)
target_compile_options(program_partitioning PRIVATE -fno-rtti -g)

target_compile_features(debug-program_partitioning PRIVATE cxx_range_for cxx_auto_type)
target_compile_options(debug-program_partitioning PRIVATE -fno-rtti -g)

