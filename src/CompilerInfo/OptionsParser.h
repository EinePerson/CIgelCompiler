//
// Created by igel on 11.11.23.
//

#ifndef IGEL_COMPILER_OPTIONSPARSER_H
#define IGEL_COMPILER_OPTIONSPARSER_H


#include <string>
#include <vector>
#include "InfoParser.h"
#include "../langMain/fileCompiler.hpp"

struct InternalInfo {
    std::vector<std::string> sourceDirs;
    std::vector<std::string> includeDirs;
    std::string mainFile;
    std::string outName;
    std::vector<SrcFile*> liveFiles;///files which are Just-in-time compiled machine files may not reference those
    std::vector<SrcFile*> files;
    //std::vector<Header*> headers;
    std::vector<Directory*> src;
    std::vector<Directory*> include;
    //std::vector<Func> calls;
    std::vector<std::string> libs {};
    SrcFile* main = nullptr;
    std::string m_name;
    std::unordered_map<std::string,SrcFile*> file_table;
    std::unordered_map<std::string,Header*> header_table;
    std::vector<std::string> linkerCommands;
    long flags = 0;
    bool link = false;

    bool hasFlag(const char * str);
};

struct Options{
    std::optional<std::string> infoFile;
    InternalInfo* info;
};

class OptionsParser {
public:

    OptionsParser(int argc,char* argv[]);

    Options* getOptions();

    InternalInfo* modify(Info* info);

    static void cmdErr(std::string err);

private:
    void setup();


    std::vector<std::string> m_options;
    std::vector<std::string> m_files;
    Options* m_opts = nullptr;
    JITFileCompiler cmp;

    void err(std::string err) {
        std::cerr << err << std::endl;
        exit(EXIT_FAILURE);
    }
};


#endif //IGEL_COMPILER_OPTIONSPARSER_H
