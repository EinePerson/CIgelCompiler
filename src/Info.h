#pragma once

#include <dirent.h>
#include <map>
#include <string>
#include <vector>
#include <unordered_map>
#include <iostream>

struct InfoSrcFile{
    std::string dir;
    std::string name;
    std::string fullName;
    bool isLive = false;
    bool isMain = false;
};

struct InfoDirectory{
    std::string name;
    std::vector<InfoDirectory*> sub_dirs;
    std::vector<InfoSrcFile*> files;
    std::vector<std::string> headers;
};

struct FileItterator{
    InfoDirectory* dir = nullptr;
    std::vector<InfoSrcFile*> files;
};

struct HeaderItterator{
    InfoDirectory* dir = nullptr;
    std::vector<std::pair<std::string,std::string>> files ;
};

inline std::string getExtension(const std::string &filename) {
    return filename.substr(filename.find_last_of('.') + 1);
}

///\brief these are all the flags that can be set
#define DEBUG_FLAG 0x1
#define OPTIMIZE_FLAG 0x2
#define NO_ARRAY_CHECK 0x4
#define NO_POINTER_CHECK 0x8

///\brief this map keeps track of the names of the specific flags
const std::map<std::string,long> FLAGS = {
    {"DEBUG_FLAG",DEBUG_FLAG},
    {"OPTIMIZE_FLAG",OPTIMIZE_FLAG},
    {"NO_ARRAY_CHECK",NO_ARRAY_CHECK},
    {"NO_POINTER_CHECK",NO_POINTER_CHECK},
};

inline FileItterator listFiles(std::string path,const std::string& name) {
    path += name;
    auto direct = new InfoDirectory;
    direct->name = name;
    std::vector<InfoSrcFile*> files;
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
                auto file = new InfoSrcFile();
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
    auto direct = new InfoDirectory;
    direct->name = name;
    std::vector<std::pair<std::string,std::string>> files;
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
                std::string name;
                name = path;
                name += f->d_name;
                direct->headers.push_back(name);
                files.push_back({name,f->d_name});
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
    std::vector<InfoDirectory*> src;
    std::vector<InfoDirectory*> include;
    std::vector<std::string> libs {};
    InfoSrcFile* main = nullptr;
    std::string m_name;
    std::unordered_map<std::string,InfoSrcFile*> file_table;
    std::unordered_map<std::string,std::string> header_table;
    std::vector<std::string> linkerCommands;
    long flags = 0;
    bool link = true;
    std::string headerEmitPath;
public:

    void addSourceDir(char* dir) __attribute__((used)) {
        sourceDirs.emplace_back(dir);
        FileItterator it = listFiles("",std::string(dir) + "/");
        /*for (auto src_file : it.files) {
            if(src_file->isLive)liveFiles.push_back(src_file);
            else files.push_back(src_file);
        }*/
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
        include.push_back(it.dir);
        header_table.reserve(it.files.size());
        for(auto file:it.files){
            header_table[file.second] = file.first;
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
        return flags & FLAGS.at(f);
    }

    void setFlag(int _flags) __attribute__((used)){
        flags = _flags;
    }

    void addLinkerFlag(char* flag) __attribute__((used)) {
        linkerCommands.emplace_back(flag);
    }

    void shouldLink(bool val){
        link = val;
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