#pragma once

#include <string>

#include "preParser.hpp"
#include "parser.hpp"
#include "../CompilerInfo/OptionsParser.h"
#include "codeGen/Generator.h"
#include "llvm/Linker/Linker.h"

class LangMain{
    public:
        explicit LangMain(Info* info,Options* options): m_info(info),m_options(options), m_tokenizer(), m_parser(info), m_gen() {
        }

        void compile(){
            for(int i = 0;i < m_info->files.size();i++){
                auto file = m_info->files.at(i);
                if(!file->tokens.empty())continue;
                auto tokens = m_tokenizer.tokenize(file->fullName);
                file->tokens = tokens;
            }
            m_parser.parse(m_info);
            for (auto src : m_info->src)src->genFile("./build/cmp/");
            if(!std::filesystem::exists("./build/cmp/")) {
                int ret = mkdir("./build/cmp/", 0777);
                if (ret != 0) {
                    std::cerr << strerror(errno);
                    exit(EXIT_FAILURE);
                }
            }

            outFiles << "clang++ -o " << m_info->m_name << " ";
            if(m_info->files.size() == 0)return;
            m_gen = new Generator(m_info->files.back());
            for(const auto file:m_info->files){
                m_gen->setup(file);
                m_gen->generate();
                m_gen->write();
                outFiles << " ./build/cmp/" << file->fullName << ".bc";
            }
            //std::cout << "Completed Assembling\n";
            std::string temp = outFiles.str();
            const char* str1 = temp.c_str();
            system(str1);
        }

private:
    /*void genAsm(SrcFile* file) {
        for (const auto &item: file->includes) {
            if (item->isGen)continue;
            genAsm(item);
        }
        m_gen->setup(file);
        m_gen->generate();
        std::string strs = "nasm -felf64 ./build/cmp/" + file->fullName + ".asm";
        outFiles << " ./build/cmp/" << file->fullName << ".o";
        const char *str = strs.c_str();
        system(str);
    }*/

    void genDir(Directory* dir) {
        int ret = mkdir("./build/cmp/", 0777);
        if (ret != 0) {
            std::cerr << strerror(errno);
            exit(EXIT_FAILURE);
        }
        for (auto sub_dir : dir->sub_dirs)genDir(sub_dir);

    }

    std::stringstream outFiles;
    Info* m_info;
    Options* m_options;
    Tokenizer m_tokenizer;
    PreParser m_parser;
    Generator* m_gen;
    std::stringstream content;
};
