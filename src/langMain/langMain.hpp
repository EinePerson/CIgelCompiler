#pragma once

#include <string>

#include "../CompilerInfo/CompilerInfo.hpp"
//#include "../tokenizer.h"
#include "preParser.hpp"
#include "parser.hpp"
#include "generator.hpp"

class LangMain{
    public:
        explicit LangMain(Info* info): m_info(info), m_tokenizer(), m_parser(info), m_gen(){}

        void compile(){
            for(int i = 0;i < m_info->files.size();i++){
                auto file = m_info->files.at(i);
                auto tokens = m_tokenizer.tokenize(file->fullName);
                file->tokens = tokens;
            }
            m_parser.parse(m_info);
            m_info->src->genFile("./build/cmp/");
            if(!std::filesystem::exists("./build/cmp/")) {
                int ret = mkdir("./build/cmp/", 0777);
                if (ret != 0) {
                    std::cerr << strerror(errno);
                    exit(EXIT_FAILURE);
                }
            }

            outFiles << "ld -o out ";
            for(auto file:m_info->files){
                m_gen.gen_prog(file);
                std::string strs = "nasm -felf64 ./build/cmp/" + file->fullName + ".asm";
                outFiles << " ./build/cmp/" << file->fullName << ".o";
                const char* str = strs.c_str();
                system(str);
            }
            std::cout << "Completed Assembling\n";
            std::string temp = outFiles.str();
            const char* str1 = temp.c_str();
            system(str1);
        }

private:
    void genAsm(SrcFile* file) {
        for (const auto &item: file->includes) {
            if (item->isGen)continue;
            genAsm(item);
        }
        m_gen.gen_prog(file);
        std::string strs = "nasm -felf64 ./build/cmp/" + file->fullName + ".asm";
        outFiles << " ./build/cmp/" << file->fullName << ".o";
        const char *str = strs.c_str();
        system(str);
    }

    std::stringstream outFiles;
    Info* m_info;
    Tokenizer m_tokenizer;
    PreParser m_parser;
    Generator m_gen;
    std::stringstream content;
};