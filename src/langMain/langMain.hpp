#pragma once

#include <string>

#include "preParser.hpp"
#include "./parsing/Parser.h"
#include "../CompilerInfo/OptionsParser.h"
#include "codeGen/Generator.h"

class LangMain{
    public:
        explicit LangMain(Info* info,Options* options): m_info(info),m_options(options), m_tokenizer(), m_parser(info), m_gen() {
        }

        void compile(){
            for(const auto file : m_info->files){
                if(!file->tokens.empty())continue;
                const auto tokens = m_tokenizer.tokenize(file->fullName);
                file->tokens = tokens;
            }
            m_parser.parse(m_info);
            for (const auto src : m_info->src)src->genFile("./build/cmp/");
            if(!std::filesystem::exists("./build/cmp/")) {
                if (const int ret = mkdir("./build/cmp/", 0777); ret != 0) {
                    std::cerr << strerror(errno);
                    exit(EXIT_FAILURE);
                }
            }

            outFiles << "clang++ -o " << m_info->m_name << " ";
            if(m_info->files.empty())return;
            m_gen = new Generator(m_info->files.back());
            for(const auto file:m_info->files){
                m_gen->setup(file);
                m_gen->generate();
                m_gen->write();
                outFiles << " ./build/cmp/" << file->fullName << ".bc";
            }
            std::string temp = outFiles.str();
            const char* str1 = temp.c_str();
            system(str1);
        }

private:

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
