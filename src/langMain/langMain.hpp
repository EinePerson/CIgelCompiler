#pragma once

#include <string>

#include "./parsing/Parser.h"
#include "../CompilerInfo/OptionsParser.h"
#include "codeGen/Generator.h"
#include "parsing/Indexer.h"
#include "parsing/PreParser.h"

//TODO Update Dependencies
const static std::vector<std::string> COMPILER_LIBS {"./libs/libigc.a","./libs/libgc.a","./libs/libgccpp.a"};


class LangMain{
    public:
        explicit LangMain(Info* info,Options* options): m_info(info),m_options(options), m_tokenizer(),m_indexer(info),m_preParser(info), m_parser(), m_gen() {
        }

        void compile(){
            for(const auto file : m_info->files){
                if(!file->tokens.empty())continue;
                const auto tokens = m_tokenizer.tokenize(file->fullName);
                file->tokens = tokens;
                m_indexer.index(file);
                m_preParser.parse(file);
                m_parser.parseProg(file);
            }

            genDir("./build");
            genDir("./build/cmp");
            for (const auto src : m_info->src)genDir("./build/cmp/",src);

            outFiles << "clang++ -o " << m_info->m_name << " ";
            if(m_info->files.empty())return;
            m_gen = new Generator(m_info->files.back(),m_info);
            for(const auto file:m_info->files){
                m_gen->setup(file);
                m_gen->generate();
                m_gen->write();
                outFiles << "./build/cmp/" << file->fullName << ".bc ";
            }
            for (const auto& lib : m_info->libs) outFiles << lib << " ";
            for (const auto& compiler_libs : COMPILER_LIBS) outFiles << compiler_libs << " ";

            std::string temp = outFiles.str();
            const char* str1 = temp.c_str();
            system(str1);
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

    std::stringstream outFiles;
    Info* m_info;
    Options* m_options;
    Tokenizer m_tokenizer;

    Indexer m_indexer;
    PreParser m_preParser;
    Parser m_parser;

    Generator* m_gen;
    std::stringstream content;
};
