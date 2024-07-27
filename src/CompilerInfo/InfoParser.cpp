//
// Created by igel on 06.11.23.
//

#include "llvm/Support/DataTypes.h"
#include "llvm/Support/ErrorHandling.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Support/ConvertUTF.h"
#include <dirent.h>
#include "InfoParser.h"

#include "../cxx_extension/CXX_Parser.h"
#include "../langMain/codeGen/Generator.h"

    InfoParser::InfoParser(std::vector<Token> tokens) : m_tokens(std::move(tokens)),m_info(nullptr) {
    }

    std::map<std::string,std::function<void(InfoParser*,std::vector<std::string>)>> InfoParser::funcs {
            {"setMain",[](InfoParser* info,std::vector<std::string> args) -> void{
                if(info->m_main.length()){
                    std::cerr << "Main file already set to:" << info->m_main << std::endl;
                    exit(EXIT_FAILURE);
                }
                info->m_main = args.at(0);
                info->m_info->file_table.at(info->m_main)->isMain = true;
            }},
            {"SourceDirectory",[](InfoParser* info,std::vector<std::string> args) -> void{
                FileItterator it = info->listFiles("",args[0] + "/");
                info->m_info->files.insert(info->m_info->files.cend(),it.files.cbegin(),it.files.cend());
                info->m_info->src.push_back(it.dir);
                info->m_info->file_table.reserve(it.files.size());
                for(auto file:it.files){
                    file->isMain = file->fullName == info->m_main;
                    info->m_info->file_table[file->fullName] = file;
                }
            }},
            {"IncludeDirectory",[](InfoParser* info,std::vector<std::string> args) -> void{
                HeaderItterator it = info->listHeaders("",args[0] + "/");
                info->m_info->headers.insert(info->m_info->headers.cend(),it.files.cbegin(),it.files.cend());
                info->m_info->include.push_back(it.dir);
                info->m_info->header_table.reserve(it.files.size());
                for(auto file:it.files){
                    //NOTIMPLEMENTEDEXCEPTION parsing for header files to include
                    //CXX_Parser(file).parseHeader();
                    info->m_info->header_table[file->fullName] = file;

                }
            }},{"SetBool",[](InfoParser* info,std::vector<std::string> args) -> void {
                if(args.size() != 2) {
                    std::cerr << "Expected 2 arguments in SetBool" << std::endl;
                    exit(EXIT_FAILURE);
                }
                if(!FLAGS.contains(args[0])) {
                    std::cerr << "Could not find setting" << std::endl;
                    exit(EXIT_FAILURE);
                }
                if(args[1] == "1Z") {
                    info->m_info->flags |= FLAGS.at(args[0]);
                }
            }},
            {"setOutName",[](const InfoParser* info,std::vector<std::string> args) -> void{
                info->m_info->m_name = args.at(0);
            }},
    };

    Info* InfoParser::parse(){
        m_info = new Info;
        std::string main = "";
        FileItterator it;
        while (peak().has_value()){
            if(auto call = parseFuncCall()){
                call.value().func(this,call.value().args);
                m_info->calls.push_back(call.value());
            }else consume();
        }
        return m_info;
    }

    std::optional<Func> InfoParser::parseFuncCall(){
        if(peak().has_value() && peak().value().type == TokenType::externFunc){
            if(!funcs.contains(peak().value().value.value())) {
                std::cerr << "Cannot Find Function " << consume().value.value() << "\n";
                exit(EXIT_FAILURE);
            }
            Func func;
            func.func =  funcs.at(consume().value.value());
            tryConsume(TokenType::openParenth,"Expected '('");
            bool b = true;
            while (peak().has_value() && (peak().value().type == TokenType::str || peak().value().type == TokenType::int_lit))
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

    HeaderItterator InfoParser::listHeaders(std::string path, const std::string& name) {
        path += name;
        auto* direct = new Directory;
        direct->name = name;
        std::vector<Header*> files;
        if (auto dir = opendir(path.c_str())) {
            while (auto f = readdir(dir)) {
                if (f->d_name[0] == '.')continue;
                if (f->d_type == DT_DIR){
                    std::string str = f->d_name;
                    str += "/";
                    auto it = listHeaders(path ,str);
                    it.dir->name = f->d_name;
                    files.insert(files.cend(),it.files.cbegin(),it.files.cend());
                    direct->sub_dirs.push_back(it.dir);
                }

                if (f->d_type == DT_REG){
                    auto file = new Header;
                    file->fullName = path;
                    file->fullName += f->d_name;
                    direct->headers.push_back(file);
                    files.push_back(file);
                }
            }
            closedir(dir);
        }else{
            std::cerr << "Could not open folder:" << name << std::endl;
            exit(EXIT_FAILURE);
        }
        HeaderItterator it;
        it.files = files;
        it.dir = direct;
        return it;
    }

    FileItterator InfoParser::listFiles(std::string path,const std::string& name) {
        path += name;
        auto* direct = new Directory;
        direct->name = name;
        std::vector<SrcFile*> files;
        if (auto dir = opendir(path.c_str())) {
            while (auto f = readdir(dir)) {
                if (f->d_name[0] == '.')continue;
                if (f->d_type == DT_DIR){
                    std::string str = f->d_name;
                    str += "/";
                    auto it = listFiles(path ,str);
                    it.dir->name = f->d_name;
                    files.insert(files.cend(),it.files.cbegin(),it.files.cend());
                    direct->sub_dirs.push_back(it.dir);
                }

                if (f->d_type == DT_REG){
                    auto file = new SrcFile();
                    file->tokenPtr = 0;
                    file->name += f->d_name;
                    file->fullName = path;
                    file->fullName += f->d_name;
                    file->dir = path;
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

    std::optional<std::pair<llvm::FunctionCallee,bool>> Header::findFunc(std::string name,std::vector<llvm::Type*> types) {
        if(funcs.contains(name)) {
            IgFunction* func = funcs[name];
            Type* t = func->_return;
            llvm::FunctionType* type = llvm::FunctionType::get(func->_return,types, false);
            return  std::make_pair(Generator::instance->m_module->getOrInsertFunction(func->mangle(),type),func->returnSigned);
        }
        return {};
    }

std::optional<IgFunction*> Header::findIgFunc(std::string name, std::vector<llvm::Type*> types) {
        if(funcs.contains(name)) return  funcs[name];
        return {};
}

std::optional<std::pair<llvm::StructType*,Struct*>> Header::findStruct(std::string name,llvm::IRBuilder<>* builder) {
    if(structs.contains(name)) return std::make_pair(structs[name]->getStrType(builder),structs[name]);
    return {};
}

std::optional<std::pair<llvm::StructType*, Class*>> Header::findClass(std::string name,llvm::IRBuilder<>* builder) {
        if(classes.contains(name))return std::make_pair(classes[name]->strType,classes[name]);
        return {};
}

std::optional<Interface *> Header::findInterface(std::string name) {
        if(interfaces.contains(name))return interfaces[name];
        return {};
}

std::optional<IgType*> Header::findContained(const std::string &name) {
        if(nameTypeMap.contains(name))return nameTypeMap[name];
        return {};
}

std::optional<IgType *> Header::findUnmangledContained(const std::string &name) {
        return {};
}


std::optional<std::pair<llvm::FunctionCallee,bool>> SrcFile::findFunc(std::string name,std::vector<Type*> types) {
        if(funcs.contains(name)) {
            IgFunction* func = funcs[name];
            llvm::FunctionType* type = llvm::FunctionType::get(func->_return,types, false);
            return  std::make_pair(Generator::instance->m_module->getOrInsertFunction(func->mangle(),type),func->returnSigned);
        }
        for (const auto &item: includes){
            if(auto ret = item->findFunc(name,types)){
                return ret;
            }
        }
        for (const auto &item: _using){
            if(auto ret = item->findFunc(name,types)){
                return ret;
            }
        }
        if(Tokenizer::LIB_FUNCS.contains(name)) {
            //TODO ADD CHECKING WEATHER FUNCTION RETURN TYPE IS SIGNED
            return  std::make_pair(Tokenizer::LIB_FUNCS.at(name)(),false);
        }

        return {};
    }

std::optional<IgFunction*> SrcFile::findIgFunc(std::string name, std::vector<llvm::Type*> types) {
        if(funcs.contains(name)) {
            return  funcs[name];
        }
        for (const auto &item: includes){
            if(auto ret = item->findIgFunc(name,types)){
                return ret;
            }
        }
        for (const auto &item: _using){
            if(auto ret = item->findIgFunc(name, types)){
                return ret;
            }
        }

        return {};
}

std::optional<std::pair<llvm::StructType*,Struct*>> SrcFile::findStruct(std::string name,llvm::IRBuilder<>* builder) {
        if(structs.contains(name)) {
            return structs.at(name);
        }
        for (const auto &item: includes){
            if(auto ret = item->findStruct(name,builder)){
                return ret;
            }
        }
        for (const auto &item: _using){
            if(auto ret = item->findStruct(name,builder)){
                return ret;
            }
        }
        return {};
    }

std::optional<std::pair<llvm::StructType*, Class*>> SrcFile::findClass(std::string name,llvm::IRBuilder<>* builder) {
        if(classes.contains(name)) {
            return classes.at(name);
        }
        for (const auto &item: includes){
            if(auto ret = item->findClass(name,builder)){
                return ret;
            }
        }
        for (const auto &item: _using){
            if(auto ret = item->findClass(name,builder)){
                return ret;
            }
        }
        return {};
}

std::optional<Interface*> SrcFile::findInterface(std::string name) {
        if(interfaces.contains(name)) {
            return interfaces.at(name);
        }
        for (const auto &item: includes){
            if(auto ret = item->findInterface(name)){
                return ret;
            }
        }
        for (const auto &item: _using){
            if(auto ret = item->findInterface(name)){
                return ret;
            }
        }
        return {};
}

std::optional<IgType *> SrcFile::findContained(std::string name) {
        if(nameTypeMap.contains(name))return nameTypeMap[name];
        else {
            for (const auto &item: includes){
                if(auto ret = item->findContained(name)){
                    return ret;
                }
            }
            for (const auto &item: _using){
                if(auto ret = item->findContained(name)){
                    return ret;
                }
            }
        }

        return {};
}

std::optional<IgType *> SrcFile::findUnmangledContained(std::string name) {
        if(unmangledTypeMap.contains(name))return unmangledTypeMap[name];
        else {
            for (const auto &item: includes){
                if(auto ret = item->findUnmangledContained(name)){
                    return ret;
                }
            }
            for (const auto &item: _using){
                if(auto ret = item->findUnmangledContained(name)){
                    return ret;
                }
            }
        }

        return {};
}

void Directory::genFile(std::string path) {
        if(!mkdir(path.c_str(),0777)){
        }
    }

bool Info::hasFlag(const std::string&flag) const {
        return (flags & FLAGS.at(flag)) == FLAGS.at(flag);
}

std::string removeExtension(const std::string &filename) {
    size_t lastdot = filename.find_last_of('.');
    if(lastdot == std::string::npos)return filename;
    return filename.substr(0, lastdot);
}
