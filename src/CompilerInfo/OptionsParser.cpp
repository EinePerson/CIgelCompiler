//
// Created by igel on 11.11.23.
//

#include <cstring>
#include "OptionsParser.h"
#include <getopt.h>
#include "../cxx_extension/CXX_Parser.h"
#include "../Info.h"

std::map<int,std::function<void(Options*)>> opts{
    {'h',[](Options* opts) -> void{

    }},
    {'o',[](Options* opts) -> void {
        /*if(optarg)opts->info->m_name = optarg;
        else; //TODO err*/
    }}
};

std::map<int,std::function<void(Options*,std::optional<std::string>)>> long_opts{

};

const struct option long_vals[] = {
        //{"o",optional_argument,0,'o'},
    {"gaming",required_argument,0,1},
{0,0,0,0}
};

std::string getName(const std::string &filename) {
    size_t lastdot = filename.find_last_of('/');
    if(lastdot == std::string::npos)return filename;
    return filename.substr(lastdot,filename.length());
}

OptionsParser::OptionsParser(int argc, char* argv[]) : m_opts(new Options()),cmp(nullptr){
    int opt;
    std::stringstream options;
    options << ":";
    for (const auto& [fst, snd] : opts)options << fst;

    int id = 0;
    while((opt = getopt_long(argc,argv,options.str().c_str(),long_vals,&id)) != -1) {
        if(opt == '?') {
            if(opts.contains(optopt)) {
                opts[optopt](m_opts);
            }else {
                std::cerr << "Unknown option " << optopt << std::endl;
                exit(EXIT_FAILURE);
            }
        }else {
            if(long_opts.contains(opt)) {
                if(optarg)
                    long_opts[opt](m_opts,optarg);
                else
                    long_opts[opt](m_opts,{});
            }else {
                std::cerr << "Unknown option " << opt << std::endl;
                exit(EXIT_FAILURE);
            }
        }
    }

    std::vector<SrcFile*> files;
    std::unordered_map<std::string,SrcFile*> file_table;
    for(; optind < argc; optind++){
        std::filesystem::path p(argv[optind]);
        if(!exists(p)) {
            std::cerr << "Unkown file " << p << std::endl;
            exit(EXIT_FAILURE);
        }

        auto file = new SrcFile;
        file->dir = "./";
        file->name = argv[optind];
        file->fullName = argv[optind];
        file->isLive = true;
        file_table[argv[optind]] = file;
        files.push_back(file);
    }
    auto header = new Header;
    header->fullName = "src/Info.h";
    std::unordered_map<std::string,Header*> list;
    list["Info.h"] = header;

    CXX_Parser(header).parseHeader();

    cmp.init(files,file_table,{header},list);
    cmp.setLiveFiles(files);
    cmp.tokenize();
    std::vector<SrcFile*> extFiles {};
    for (auto src_file : cmp.files) {
        if(src_file->tokens.front().type != TokenType::info && src_file->tokens.front().value.value() != "info") {
            auto range = std::ranges::remove(cmp.files,src_file);
            cmp.files.erase(range.begin(),range.end());
            extFiles.push_back(src_file);
        }
    }

    cmp.addHeader("src/Info.h");
    //cmp.addHeader("src/CompilerInfo/Info.cpp");
    cmp.preParse();
    cmp.parse();
    cmp.preGen();
    cmp.gen();
    cmp.compileHeaders();
    cmp.link();

    auto exb = cmp.jit->lookupLinkerMangled("_Z5buildP4Info");
    auto exp = cmp.jit->lookup("_Z5buildP4Info");

    if(!exp) {
        if(exp.errorIsA<orc::SymbolsNotFound>())
            outs() << "Build file has to have function build(Info)\n";
        else outs() << exp.takeError();
        exit(EXIT_FAILURE);
    }
    auto mainFunc = exp.get().toPtr<int(Info*)>();
    auto ret = mainFunc(m_opts->info);

    modify(m_opts->info);
    if(ret != 0) {
        std::cerr << "Build script exited with: " << ret << std::endl;
        exit(EXIT_FAILURE);
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

void OptionsParser::modify(Info* info) {
    if(info == nullptr)return;
    for (const auto &item: m_files){
        auto file = new SrcFile;
        file->tokenPtr = 0;
        file->name += getName(item);
        file->fullName = item;
        file->isLive = getExtension(file->name).contains('l');
        //file->fullNameOExt += removeExtension(item);
        info->file_table.reserve(1);
        info->file_table[file->fullName] = file;
    }
}

void OptionsParser::setup() {

}



