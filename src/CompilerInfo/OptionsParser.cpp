//
// Created by igel on 11.11.23.
//

#include <cstring>
#include "OptionsParser.h"

std::string getName(const std::string &filename) {
    size_t lastdot = filename.find_last_of('/');
    if(lastdot == std::string::npos)return filename;
    return filename.substr(lastdot,filename.length());
}

OptionsParser::OptionsParser(int argc, char* argv[]){
    for (int i = 1; i < argc; ++i){
        if(strlen(argv[i]) <= 0)continue;
        if(argv[i][0] == '-'){
            m_options.emplace_back(argv[i] + 1, strlen(argv[i] + 1));
        }else{
            m_files.emplace_back(argv[i], strlen(argv[i]));
        }
    }
}

Options *OptionsParser::getOptions() {
    auto opts = new Options;
    Tokenizer tokenizer;
    for (const auto &item: m_files){
        std::vector<Token> tokens = tokenizer.tokenize(item);
        if(!tokens.empty() && tokens[0].type == TokenType::info && tokens[0].value == "info"){
            if(!opts->infoFile){
                opts->infoFile = item;
                std::remove(m_files.begin(), m_files.end(),item);
            }else{
                std::cerr << "Function file already defined as: " << opts->infoFile.value() << "\n at: " << item;
                exit(EXIT_FAILURE);
            }
        }
    }
    return opts;
}

void OptionsParser::modify(Info *info) {
    if(info == nullptr)return;
    for (const auto &item: m_files){
        auto file = new SrcFile;
        file->tokenPtr = 0;
        file->name += removeExtension(getName(item));
        file->fullName = item;
        file->fullNameOExt += removeExtension(item);
        info->file_table.reserve(1);
        info->file_table[file->fullName] = file;
    }
}



