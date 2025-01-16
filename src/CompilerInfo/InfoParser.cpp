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
#include "../types.h"
#include "../Info.h"

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

Header::Header(std::string second) : fullName(second) {
}

SrcFile::SrcFile(InfoSrcFile *src) : dir(src->dir), name(src->name),fullName(src->fullName),isLive(src->isLive),isMain(src->isMain) {
}

SrcFile::SrcFile() {
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

Directory::Directory(InfoDirectory *dir):name(dir->name) {
        sub_dirs.reserve(dir->sub_dirs.size());
        for (const auto &item: dir->sub_dirs) {
            sub_dirs.push_back(new Directory(item));
        }
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