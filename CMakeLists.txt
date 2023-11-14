cmake_minimum_required(VERSION 3.20)

project(igel_compiler)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_BINARY_DIR ${CMAKE_SOURCE_DIR}/build/libs)

add_executable(igel
        src/main.cpp
        src/tokenizer.cpp
        src/tokenizer.h
        src/parsed.h
        src/CompilerInfo/InfoParser.cpp
        src/CompilerInfo/InfoParser.h
        src/CompilerInfo/OptionsParser.h
        src/CompilerInfo/OptionsParser.cpp
)
set_target_properties(igel PROPERTIES OUTPUT_NAME "igel.exe")