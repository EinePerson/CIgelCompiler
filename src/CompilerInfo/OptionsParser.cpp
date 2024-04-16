//
// Created by igel on 11.11.23.
//

#include <cstring>
#include "OptionsParser.h"

std::map<char,std::function<void(Options*)>> opts{
    {'h',[](Options* opts) -> void{

    }},
    {'o',[](Options* opts) -> void {
        opts->info->m_name = optarg;
    }}
};

std::string getName(const std::string &filename) {
    size_t lastdot = filename.find_last_of('/');
    if(lastdot == std::string::npos)return filename;
    return filename.substr(lastdot,filename.length());
}

OptionsParser::OptionsParser(int argc, char* argv[]) : m_opts(new Options){
    int opt;
    std::stringstream options;
    options << ":";
    for (auto [fst, snd] : opts)options << fst;
    while((opt = getopt(argc,argv,options.str().c_str())) != -1) {
        opts[opt](m_opts);
    }
    for(; optind < argc; optind++){
        std::filesystem::path p(argv[optind]);
        if(p.extension() == ".ig") {
            auto file = new SrcFile;
            file->tokenPtr = 0;
            file->name = p.replace_extension("");
            file->fullName = p;
            file->fullNameOExt = p.replace_extension("");
            m_opts->info->files.push_back(file);
        }else if(p.extension() == ".igc") {
            if(m_opts->infoFile) {
                std::cerr << "cannot have one than more info file" << std::endl;
                exit(EXIT_FAILURE);
            }
            m_opts->infoFile = p;
        }
    }
}

Options *OptionsParser::getOptions() {
    Tokenizer tokenizer;
    for (const auto &item: m_files){
        std::vector<Token> tokens = tokenizer.tokenize(item);
        if(!tokens.empty() && tokens[0].type == TokenType::info && tokens[0].value == "info"){
            if(!m_opts->infoFile){
                m_opts->infoFile = item;
                std::remove(m_files.begin(), m_files.end(),item);
            }else{
                std::cerr << "Function file already defined as: " << m_opts->infoFile.value() << "\n at: " << item;
                exit(EXIT_FAILURE);
            }
        }
    }
    return m_opts;
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



