#pragma once

#include <dirent.h>
#include <map>
#include <string>
#include <vector>
#include <unordered_map>
#include <iostream>
#include "./CompilerInfo/InfoParser.h"

///\brief these are all the flags that can be set
#define DEBUG_FLAG 0x1
#define OPTIMIZE_FLAG 0x2

///\brief this map keeps track of the names of the specific flags
const std::map<std::string,long> FLAGS = {
    {"Debug",DEBUG_FLAG},
    {"Optimize",OPTIMIZE_FLAG},
};

inline FileItterator listFiles(std::string path,const std::string& name) {
    path += name;
    auto direct = new Directory;
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
                file->isLive = getExtension(file->name).back() == 'l';
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

inline HeaderItterator listHeaders(std::string path, const std::string& name) {
    path += name;
    auto direct = new Directory;
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

/**
 * \brief this file is used to construct the compiler info in a build file, or from command line options
 */
class Info {
public:
    std::vector<std::string> sourceDirs;
    std::vector<std::string> includeDirs;
    std::string mainFile;
    std::string outName;
    std::vector<SrcFile*> liveFiles;///files which are Just-in-time compiled machine files may not reference those
    std::vector<SrcFile*> files;
    std::vector<Header*> headers;
    std::vector<Directory*> src;
    std::vector<Directory*> include;
    std::vector<Func> calls;
    std::vector<std::string> libs {};
    SrcFile* main = nullptr;
    std::string m_name;
    std::unordered_map<std::string,SrcFile*> file_table;
    std::unordered_map<std::string,Header*> header_table;
    long flags = 0;
public:

    void addSourceDir(char* dir) __attribute__((used)) {
        sourceDirs.emplace_back(dir);
        FileItterator it = listFiles("",std::string(dir) + "/");
        for (auto src_file : it.files) {
            if(src_file->isLive)liveFiles.push_back(src_file);
            else files.push_back(src_file);
        }
        src.push_back(it.dir);
        file_table.reserve(it.files.size());
        for(auto file:it.files){
            file->isMain = file->fullName == mainFile;
            if(file->isMain)main = file;
            file_table[file->fullName] = file;
        }
    }

    void addIncludeDir(char* dir) __attribute__((used)) {
        includeDirs.emplace_back(dir);
        HeaderItterator it = listHeaders("",std::string(dir) + "/");
        headers.insert(headers.cend(),it.files.cbegin(),it.files.cend());
        include.push_back(it.dir);
        header_table.reserve(it.files.size());
        for(auto file:it.files){
            header_table[file->fullName] = file;
        }
    }

    void setMainFile(char* file) __attribute__((used)) {
        mainFile = file;
        main = file_table[std::string(file)];
        if(main)main->isMain = true;
    }

    void setOutName(char* out) __attribute__((used)) {
        outName = out;
    }

    bool hasFlag(char* flag) const __attribute__((used)){
        std::string f = flag;
        /*if(!FLAGS.contains(f)) {
            std::cerr << "Unknown flag: " << f << std::endl;
            exit(EXIT_FAILURE);
        }*/
        return flags & FLAGS.at(f);
    }

    void setFlag(char* flag,bool value) __attribute__((used)){
        flags |= (FLAGS.at(flag) * value);
    }

    ///this function checks if the Info is valid, it throws errors whenever non-usable values are found
    void verify() const __attribute__((used)){
        if(mainFile.empty()) {
            err("A main file is needed");
        }
    }

private:
    static void err(const std::string& err){
        std::cerr << err << " in the build configuration" << std::endl;
        exit(EXIT_FAILURE);
    }
};