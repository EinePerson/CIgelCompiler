//
// Created by igel on 11.11.23.
//

#include <cstring>
#include "OptionsParser.h"
#include "../cxx_extension/CXX_Parser.h"
#include "../Info.h"
#include "../SelfInfo.h"
#include "../util/Mangler.h"
#include "../util/String.h"

std::map<int,std::function<void(Options*)>> opts{
    /*{'h',[](Options* opts) -> void{

    }},*/


};

std::map<int,std::function<void(InternalInfo*,std::optional<std::string>)>> short_opts{
        {'o',[](InternalInfo* info,std::optional<std::string> str) -> void {
            if(!str.has_value())OptionsParser::cmdErr("Expected output file name after '-o'");
            info->outName = str.value();
        }},
        {'L',[](InternalInfo* info,std::optional<std::string> str) -> void {
            if(!str.has_value())OptionsParser::cmdErr("Expected Linker Flag after '-L'");
            auto args = String::split(str.value()," ");
            info->linkerCommands.reserve(args.size());
            for (auto arg : args)info->linkerCommands.push_back(arg);
        }},
        {'I',[](InternalInfo* info,std::optional<std::string> str) -> void {
            if (!str.has_value())OptionsParser::cmdErr("Expected input directory after '-I'");
            OptionsParser::addIncludeDir(info,str.value());
        }},
        {'c',[](InternalInfo* info,std::optional<std::string> str) -> void {
           info->link = false;
        }},
};

std::map<std::string,std::function<void(InternalInfo*)>> long_opts{
    {"ffreestanding",[](InternalInfo* info) -> void {
       info->flags |= FREESTANDING_FLAG;
    }},
    {"fno-mangle",[](InternalInfo* info) -> void {
        Igel::Mangler::init(Igel::NONE);
    }}
};

std::map<std::string,int> long_to_short = {
    {"out",'o'},
    {"Link",'L'},
    {"Include",'I'},
};

std::map<int,bool> has_param = {
    {'o',true},
    {'L',true},
    {'I',true}
};

std::string getName(const std::string &filename) {
    size_t lastdot = filename.find_last_of('/');
    if(lastdot == std::string::npos)return filename;
    return filename.substr(lastdot,filename.length());
}

bool InternalInfo::hasFlag(const char *str) {
    return FLAGS.at(str) & flags;
}

OptionsParser::OptionsParser(int argc, char* argv[]) : m_opts(new Options()),cmp(nullptr){
    std::vector<SrcFile*> files;
    std::unordered_map<std::string,SrcFile*> file_table;

    std::vector<std::pair<std::function<void(InternalInfo*,std::optional<std::string>)>,std::optional<std::string>>> params;
    std::vector<std::function<void(InternalInfo*)>> params_long;
    std::vector<std::string> include_args;

    for (int i = 1;i < argc;i++) {
        std::string arg = argv[i];
        if (arg.starts_with("--")) {
            if (long_to_short.contains(arg.substr(2))) {
                int id = long_to_short[arg.substr(2)];
                std::optional<std::string> param;
                if (has_param[id]) {
                    if (id + 1 >= argc) {
                        param = argv[++i];
                    }else err("Expected argument after " + arg.substr(2));
                }
                params.push_back(std::make_pair(short_opts[id],param));
            }else if (long_opts.contains(arg.substr(2))) {
                params_long.push_back(long_opts[arg.substr(2)]);
            }else err("Unknown option '" + arg.substr(2) + "'");
        }else if (arg.starts_with("-")) {
            if (arg == "-I")include_args.push_back(argv[i + 1]);
            if (short_opts.contains(arg.at(1))) {
                std::optional<std::string> param;
                int id = arg.at(1);
                if (has_param[id]) {
                    if (id + 1 >= argc) {
                        param = argv[++i];
                    }else err("Expected argument after " + (char) id);
                }
                params.push_back(std::make_pair(short_opts[arg.at(1)],param));
            }else
                err("Unknown option: " +  std::string(arg));
        }else {
            std::filesystem::path p(argv[i]);
            if(!exists(p)) {
                std::cerr << "Unkown file " << p << std::endl;
                exit(EXIT_FAILURE);
            }

            auto file = new SrcFile();
            file->dir = "./";
            file->name = argv[i];
            file->fullName = argv[i];
            //file->isLive = true;
            file_table[argv[i]] = file;
            files.push_back(file);
        }
    }





    cmp.init(files,file_table,{},{});
    cmp.setLiveFiles(files);
    cmp.tokenize();

    bool hasInfo = false;
    for (auto file1 : files) {
        if (!file1->tokens.empty() && file1->tokens[0].type == TokenType::info && file1->tokens[0].value == "info") {
            hasInfo = true;
            break;
        }
    }


    if (hasInfo) {
        auto header = new Header;
        header->fullName = "./src/Info.h";

        std::unordered_map<std::string,Header*> list;
        list["Info.h"] = header;

        CXX_Parser(header,include_args,std::filesystem::current_path()).parseHeader();

        cmp.init(files,file_table,{header},{list});
    }

    std::vector<SrcFile*> extFiles {};
    for (auto src_file : cmp.files) {
        if(!src_file->tokens.empty() && src_file->tokens.front().type != TokenType::info && src_file->tokens.front().value.value() != "info") {
            auto range = std::ranges::remove(cmp.files,src_file);
            cmp.files.erase(range.begin(),range.end());
            extFiles.push_back(src_file);
        }
    }



    if (hasInfo)cmp.addHeader("./src/Info.h");
    //cmp.addHeader("src/CompilerInfo/Info.cpp");
    /*Generator::instance->m_vars.emplace_back();
    for (const auto &item: FLAGS){
        auto builder = Generator::instance->getBuilder();
        auto alloc = builder->CreateAlloca(builder->getInt64Ty());
        builder->CreateStore(ConstantInt::get(builder->getInt64Ty(),item.second),alloc);
        Var* v = new Var(alloc,false,true);
        Generator::instance->createVar(item.first,v);
    }*/
    cmp.preParse();
    cmp.parse();
    cmp.preGen();
    cmp.gen();
    cmp.compileHeaders();
    cmp.link();
    //Generator::instance->m_vars.pop_back();



    if (hasInfo) {
        auto exb = cmp.jit->lookupLinkerMangled("_Z5buildP4Info");
        auto exp = cmp.jit->lookup("_Z5buildP4Info");

        if(!exp) {
            if(exp.errorIsA<orc::SymbolsNotFound>())
                outs() << "Build file has to have function build(Info)\n";
            else outs() << exp.takeError();
            exit(EXIT_FAILURE);
        }
        auto mainFunc = exp.get().toPtr<int(Info*)>();
        auto exInfo = new Info;
        auto ret = mainFunc(exInfo);

        m_opts->info = modify(exInfo);
        delete exInfo;
        if(ret != 0) {
            std::cerr << "Build script exited with: " << ret << std::endl;
            exit(EXIT_FAILURE);
        }
    }else {
        m_opts->info = new InternalInfo;
    }

    //m_opts->info->files.insert(m_opts->info->files.end(),extFiles.begin(), extFiles.end());
    for (auto ext_file : extFiles) {
        m_opts->info->file_table[ext_file->name] = ext_file;
    }
    //m_opts->info->file_table.merge(cmp.file_table);

    for (auto [func,param] : params) {
        func(m_opts->info,param);
    }

    for (auto long_ : params_long)long_(m_opts->info);

    for (auto [name,file] : file_table) {
        if (!file->tokens.empty() && file->tokens[0].type == TokenType::info && file->tokens[0].value.value() == "info")continue;
        m_opts->info->files.push_back(file);
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

InternalInfo* OptionsParser::modify(Info* oldInfo) {
    if(oldInfo == nullptr)return nullptr;
    InternalInfo* info = new InternalInfo;
    for (auto table : oldInfo->file_table) {
        info->file_table[table.first] = new SrcFile(table.second);
        if (table.second->isLive)info->liveFiles.push_back(new SrcFile(table.second));
        else info->files.push_back(new SrcFile(table.second));
    }

    //info->headers.reserve(oldInfo->header_table.size());
    for (auto table : oldInfo->header_table) {
        info->header_table[table.first] = new Header(table.second);
        //info->headers.push_back(new Header(table.first));
    }

    info->flags = oldInfo->flags;
    info->libs = oldInfo->libs;
    info->link = oldInfo->link;
    info->outName = oldInfo->outName;
    info->sourceDirs = oldInfo->sourceDirs;
    info->mainFile = oldInfo->mainFile;
    info->main = info->file_table[oldInfo->mainFile];
    info->linkerCommands = oldInfo->linkerCommands;
    info->includeDirs = oldInfo->includeDirs;
    for (auto include : oldInfo->include) {
        info->include.push_back(new Directory(include));
    }
    for (auto src : oldInfo->src) {
        info->src.push_back(new Directory(src));
    }

    return info;

    /*for (const auto &item: m_files){
        auto file = new SrcFile;
        file->tokenPtr = 0;
        file->name += getName(item);
        file->fullName = item;
        file->isLive = getExtension(file->name).contains('l');
        //file->fullNameOExt += removeExtension(item);
        info->file_table.reserve(1);
        info->file_table[file->fullName] = file;
    }*/
}

void OptionsParser::setup() {

}

void OptionsParser::cmdErr(std::string err) {
    std::cerr << "Error in command line args: " << err << std::endl;
    exit(EXIT_FAILURE);
}



