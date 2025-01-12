#pragma once

#include <string>
#include <llvm/ExecutionEngine/Orc/ThreadSafeModule.h>

#include "fileCompiler.hpp"
#include "./parsing/Parser.h"
#include "../CompilerInfo/OptionsParser.h"
#include "codeGen/Generator.h"
#include "parsing/Indexer.h"
#include "parsing/PreParser.h"
#include "../cxx_extension/CXX_Parser.h"
//#include "../JIT_unit/JIT.h"

//TODO Update Dependencies
const static std::vector<std::string> COMPILER_LIBS {"./libs/libigc.a","./libs/libgc.a","./libs/libgccpp.a"/*, "-Wl,--whole-archive ./build/cmp/includes.o"*/};


class LangMain{
public:
    explicit LangMain(InternalInfo* info,Options* options): m_info(info),m_options(options),//cmp(m_options->info,info->files,info->file_table,info->headers,info->header_table),
    jit(m_options->info,info->files,info->liveFiles,info->file_table/*,info->headers*/,info->header_table)
        {

    }

    void compile(){
        for (auto header : m_info->header_table) {
            CXX_Parser(header.second).parseHeader();
        }

        jit.tokenize();
        jit.preParse();

        jit.parse();

        genDir("./build");
        genDir("./build/cmp");
        for (const auto src : m_info->src)genDir("./build/cmp/",src);
        for (const auto src : m_info->include)genDir("./build/cmp/",src);

        outFiles << "clang++ -o " << m_info->outName;
        if(!m_info->main || (!m_info->main && m_info->main->isLive))outFiles << ".so -shared ";
        else outFiles << " ";
        if(m_info->files.empty())return;
        jit.preGen();

        jit.genNonLive([this](SrcFile* file,FileCompiler* cmp) -> void {
            cmp->m_gen->write();
            outFiles << "./build/cmp/" << file->fullName << ".bc ";
        });

        for (auto header : m_info->header_table) {
            outFiles << "./build/cmp/" << header.second->fullName << ".pch ";
            std::string cmdStr = "clang++ -g -o ./build/cmp/";
            cmdStr += header.second->fullName + ".pch ";
            cmdStr += header.second->fullName;
            const char* cmd = cmdStr.c_str();
            if(system(cmd))err();
        }
        for (const auto& lib : m_info->libs) outFiles << lib << " ";
        for (const auto& compiler_libs : COMPILER_LIBS) outFiles << compiler_libs << " ";

        for (const auto &item: m_info->linkerCommands)outFiles << "-" << item;

        if(!m_info->files.empty()) {
            outFiles << " -std=c++23 -lstdc++exp -fsized-deallocation ";
            std::string temp = outFiles.str();
            const char* str1 = temp.c_str();
            if(system(str1)) {
                err();
            }
        }

        jit.genLive();
        jit.compileHeaders();
        jit.link();
    }

    void exec() {
        if(m_info->main && !m_info->main->isLive)return;
        int result = jit.execute("main");

        std::cout << "\nProcess finished with exit code: " << result << std::endl;
    }

    ~LangMain() = default;

private:

    void genDir(std::string path,Directory* dir) {
        std::string name = path + "/" + dir->name;
        if(!std::filesystem::exists(name)){
            int ret = mkdir(name.c_str(), 0777);
            if (ret != 0) {
                std::cerr << strerror(errno);
                exit(EXIT_FAILURE);
            }
        }
        for (auto sub_dir : dir->sub_dirs)genDir(name,sub_dir);

    }

    void genDir(std::string name) {
        if(!std::filesystem::exists(name)){
            int ret = mkdir(name.c_str(), 0777);
            if (ret != 0) {
                std::cerr << strerror(errno);
                exit(EXIT_FAILURE);
            }
        }
    }

    void err() {
        exit(EXIT_FAILURE);
    }

    std::stringstream outFiles;
    InternalInfo* m_info;
    Options* m_options;

    //FileCompiler cmp;
    JITFileCompiler jit;
};
