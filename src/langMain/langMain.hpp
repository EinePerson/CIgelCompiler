#pragma once

#include <string>
#include <clang/AST/RecursiveASTVisitor.h>
#include <llvm/ExecutionEngine/Orc/ThreadSafeModule.h>

#include "fileCompiler.hpp"
#include "./parsing/Parser.h"
#include "../CompilerInfo/OptionsParser.h"
#include "codeGen/Generator.h"
#include "parsing/Indexer.h"
#include "parsing/PreParser.h"
#include "../cxx_extension/CXX_Parser.h"
#include "../util/String.h"
//#include "../JIT_unit/JIT.h"
#include "../SelfInfo.h"

//TODO Update Dependencies
const static std::vector<std::string> COMPILER_LIBS {"/usr/lib/IGC-libigc.a","/usr/lib/IGC-libgc.a","/usr/lib/IGC-libgccpp.a"/*, "-Wl,--whole-archive ./build/cmp/includes.o"*/};


class LangMain{
public:
    explicit LangMain(InternalInfo* info,Options* options): m_info(info),m_options(options),//cmp(m_options->info,info->files,info->file_table,info->headers,info->header_table),
    jit(m_options->info,info->files,info->liveFiles,info->file_table/*,info->headers*/,info->header_table)
        {

    }

    void compile(){
        //std::vector<std::string> includeArgs;
        //includeArgs.reserve(m_info->includeDirs.size());
        //for (auto include_dir : m_info->includeDirs)includeArgs.push_back("-I " + include_dir);
        for (auto header : m_info->header_table) {
            CXX_Parser(header.second,m_info->includeDirs,std::filesystem::current_path()).parseHeader();
        }

        jit.tokenize();
        jit.preParse();

        jit.parse();

        outFiles << "clang++ -o " << m_info->outName;

        //if (m_info->link) {
            genDir("./build");
            genDir("./build/cmp");
            //for (const auto src : m_info->src)genDir("./build/cmp",src);
            for (const auto src : m_info->include)genDir("./build/cmp",src);
        //}else
        if (!m_info->link){
            outFiles << " -c";
        }

        //TODO change shared library setting
        //if(!m_info->main || (!m_info->main && m_info->main->isLive))outFiles << ".so -shared ";
        //else outFiles << " ";
        outFiles << " ";
        if(m_info->files.empty() && m_info->link)return;
        jit.preGen();

        jit.genNonLive([this](SrcFile* file,FileCompiler* cmp) -> void {
            genDir(std::string(std::filesystem::current_path()) + "/build/cmp",file->fullName,true);
            cmp->m_gen->write();
            outFiles << "./build/cmp/" << file->fullName << ".bc ";
        });

        if ((m_info->flags & EMIT_CPP != 0) || (m_info->flags & EMIT_C != 0)) {
            genDir(m_info->headerEmitPath);
            if ((m_info->flags & EMIT_CPP) != 0)jit.emitCPP(m_info->headerEmitPath);
            else if ((m_info->flags & EMIT_C) != 0)jit.emitC(m_info->headerEmitPath);
        }

        /*for (auto header : m_info->header_table) {
            outFiles << "./build/cmp/" << header.second->fullName << ".pch ";
            std::string cmdStr = "clang++ -g -o ./build/cmp/";
            cmdStr += header.second->fullName + ".pch ";
            cmdStr += header.second->fullName;
            const char* cmd = cmdStr.c_str();
            if(system(cmd))err();
        }*/


        for (const auto& lib : m_info->libs) outFiles << lib << " ";
        if ((m_info->flags & FREESTANDING_FLAG) == 0)
            for (const auto& compiler_libs : COMPILER_LIBS) outFiles << compiler_libs << " ";

        for (const auto &item: m_info->linkerCommands)outFiles << " " << item << " ";

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
        if((m_info->main && !m_info->main->isLive) || !m_info->main)return;
        int result = jit.execute("main");

        std::cout << "\nProcess finished with exit code: " << result << std::endl;
    }

    ~LangMain() = default;

private:

    void genDir(std::vector<std::string> dirs,int id) {
        if (dirs.size() <= id)return;
        if(!std::filesystem::exists(dirs[id])){
            int ret = mkdir(dirs[id].c_str(), 0777);
            if (ret != 0) {
                std::cerr << strerror(errno);
                exit(EXIT_FAILURE);
            }
        }

        genDir(dirs,id + 1);
    }

    void genDir(std::string startStr,std::string path,bool hasFile = false) {
        std::vector<std::string> dirs;
        std::vector<std::string> split = String::split(path,"/");
        if (hasFile)split.pop_back();
        startStr += "/";
        int start = 0;
        for (int i = 0;i < split.size();i++) {
            bool found = false;
            for (int j = 0;j < split[i].size();j++) {
                if (std::isalnum(split[i][j])) {
                    found = true;
                    break;
                }
            }

            if (found) {
                std::string dir = dirs.empty()?startStr:dirs.back();
                for (int k = start;k <= i;k++) {
                    dir += split[k] + "/";
                }

                start = i + 1;
                dirs.push_back(dir);
            }
        }

        genDir(dirs,0);
    }

    void genDir(std::string path,Directory* dir) {
        genDir(path,dir->name);
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
