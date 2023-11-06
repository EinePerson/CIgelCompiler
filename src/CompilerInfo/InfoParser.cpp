//
// Created by igel on 06.11.23.
//

#include <dirent.h>
#include "InfoParser.h"

    InfoParser::InfoParser(std::vector<Token> tokens,ArenaAlocator* alloc) : m_tokens(std::move(tokens)) {
        m_alloc = alloc;
    }

    std::map<std::string,std::function<void(InfoParser*,std::vector<std::string>)>> InfoParser::funcs {
            {"setMain",[](InfoParser* info,std::vector<std::string> args) -> void{
                if(info->m_main.length()){
                    std::cerr << "Main file already set to:" << info->m_main << std::endl;
                    exit(EXIT_FAILURE);
                }
                info->m_main = args.at(0);
                info->m_info->file_table[info->m_main]->isMain = true;
            }},
            {"setSourceDirectory",[](InfoParser* info,std::vector<std::string> args) -> void{
                FileItterator it = info->listFiles("",args[0] + "/");
                info->m_info->files = it.files;
                info->m_info->src = it.dir;
                info->m_info->file_table.reserve(it.files.size());
                for(auto file:it.files){
                    file->isMain = file->fullName == info->m_main;
                    info->m_info->file_table[file->fullName] = file;
                }
            }},
            {"setOutName",[](const InfoParser* info,std::vector<std::string> args) -> void{
                info->m_info->m_name = &args.at(0);
            }},
    };

    Info* InfoParser::parse(){
        m_info = m_alloc->alloc<Info>();
        std::string main = "";
        FileItterator it;
        while (auto call = parseFuncCall())
        {
            call.value().func(this,call.value().args);
            m_info->calls.push_back(call.value());
        }
        /*std::unordered_map<std::string,size_t> file_I;
        m_info->files = it.files;
        m_info->src = it.dir;
        m_info->file_table.reserve(it.files.size());
        for(auto file:it.files){
            file->isMain = file->fullName == main;
            m_info->file_table[file->fullName] = file;
        }*/
        return m_info;
    }

    std::optional<Func> InfoParser::parseFuncCall(){
        if(peak().has_value() && peak().value().type == TokenType::externFunc){
            Func func;
            func.func =  funcs.at(consume().value.value());
            tryConsume(TokenType::openParenth,"Expected '('");
            bool b = true;
            while (peak().has_value() && peak().value().type == TokenType::str)
            {
                if(!b)tryConsume(TokenType::comma,"Expected ','");
                func.args.push_back(consume().value.value());
                b = tryConsume(TokenType::comma).has_value();
            }

            tryConsume(TokenType::closeParenth,"Expected ')'");
            tryConsume(TokenType::semi,"Expected ';'");
            return func;
        }else return {};

    }

    FileItterator InfoParser::listFiles(std::string path,const std::string& name) {
        path += name;
        Directory* direct = m_alloc->alloc<Directory>();
        direct->name = name;
        std::vector<SrcFile*> files;
        if (auto dir = opendir(path.c_str())) {
            while (auto f = readdir(dir)) {
                if (!f->d_name || f->d_name[0] == '.')continue;
                if (f->d_type == DT_DIR){
                    std::string str = f->d_name;
                    str += "/";
                    auto it = listFiles(path ,str);
                    it.dir->name = f->d_name;
                    files.insert(files.cend(),it.files.cbegin(),it.files.cend());
                    direct->sub_dirs.push_back(it.dir);
                }

                if (f->d_type == DT_REG){
                    auto file = m_alloc->alloc<SrcFile>();
                    file->tokenPtr = 0;
                    file->name += removeExtension(f->d_name);
                    file->fullName = path;
                    file->fullName += f->d_name;
                    file->fullNameOExt = path;
                    file->fullNameOExt += removeExtension(f->d_name);
                    direct->files.push_back(file);
                    files.push_back(file);
                }
            }
            closedir(dir);
        }else{
            std::cerr << "Could not open folder:" << name << std::endl;
            exit(EXIT_FAILURE);
        }
        FileItterator it;
        it.files = files;
        it.dir = direct;
        return it;
    }

    std::optional<FuncRet> SrcFile::findFunc(std::string name) {
        if(funcs.contains(name))return FuncRet{.func = funcs[name],.state = FunctionState::_inside};
        for (const auto &item: includes){
            if(auto ret = item->findFunc(name)){
                ret->state = FunctionState::_include;
                return ret;
            }
        }
        for (const auto &item: _using){
            if(auto ret = item->findFunc(name)){
                ret->state = FunctionState::_use;
                return ret;
            }
        }

        return {};
    }

    void Directory::genFile(std::string path) {
        if(!mkdir(path.c_str(),0777)){
        }
    }

std::string removeExtension(const std::string &filename) {
    size_t lastdot = filename.find_last_of('.');
    if(lastdot == std::string::npos)return filename;
    return filename.substr(0, lastdot);
}
