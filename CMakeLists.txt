cmake_minimum_required(VERSION 3.20.0)

project(igel_compiler)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_BINARY_DIR ${CMAKE_SOURCE_DIR}/build/libs)
set_property(GLOBAL PROPERTY TARGET_SUPPORTS_SHARED_LIBS TRUE)

add_executable(igel
        src/main.cpp
        src/tokenizer.cpp
        src/tokenizer.h
        src/CompilerInfo/InfoParser.cpp
        src/CompilerInfo/InfoParser.h
        src/CompilerInfo/OptionsParser.h
        src/CompilerInfo/OptionsParser.cpp
        src/langMain/codeGen/Generator.h
        src/langMain/codeGen/Generator.cpp
        src/langMain/codeGen/types.cpp
        src/langMain/parsing/Parser.cpp
        src/util/Mangler.h
        src/exceptionns/Generator.h
        src/util/Mangler.cpp
        src/langMain/parsing/Indexer.cpp
        src/langMain/parsing/Indexer.h
        src/langMain/parsing/PreParser.cpp
        src/langMain/parsing/PreParser.h
)
set_target_properties(igel PROPERTIES OUTPUT_NAME "igelCompiler")

#This is the LLVM Dependency section

find_package(LLVM REQUIRED CONFIG)

message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")
message(STATUS "Using LLVMConfig.cmake in: ${LLVM_DIR}")

# Set your project compile flags.
# E.g. if using the C++ header files
# you will need to enable C++11 support
# for your compiler.

add_definitions(${LLVM_DEFINITIONS_LIST})
include_directories(${LLVM_INCLUDE_DIRS})
link_directories(${LLVM_LIBRARY_DIRS})
separate_arguments(LLVM_DEFINITIONS_LIST NATIVE_COMMAND ${LLVM_DEFINITIONS})

# Now build our tools

# Find the libraries that correspond to the LLVM components
# that we wish to use
llvm_map_components_to_libnames(llvm_libs support core irreader)

# Link against LLVM libraries
target_link_libraries(igel /usr/lib/libLLVM-17.so)
#target_link_libraries(igel ${llvm_libs})

SET(libSrcPr
         srcLib/Array.cpp)

add_library(libIg STATIC ${libSrcPr})
set_target_properties(libIg PROPERTIES OUTPUT_NAME "igc")
set_target_properties( libIg
        PROPERTIES
        ARCHIVE_OUTPUT_DIRECTORY "./libs"
        LIBRARY_OUTPUT_DIRECTORY "./libs"
        RUNTIME_OUTPUT_DIRECTORY "./bin"
)
#set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
#set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)