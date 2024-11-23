//
// Created by igel on 11.11.23.
//

#ifndef IGEL_COMPILER_OPTIONSPARSER_H
#define IGEL_COMPILER_OPTIONSPARSER_H


#include <string>
#include <vector>
#include "InfoParser.h"
#include "../Info.h"
#include "../langMain/fileCompiler.hpp"

struct Options{
    std::optional<std::string> infoFile;
    Info* info = new Info;
};

class OptionsParser {
public:

    OptionsParser(int argc,char* argv[]);

    Options* getOptions();

    void modify(Info* info);

    static void cmdErr(std::string err);

private:
    void setup();


    std::vector<std::string> m_options;
    std::vector<std::string> m_files;
    Options* m_opts = nullptr;
    JITFileCompiler cmp;
};


#endif //IGEL_COMPILER_OPTIONSPARSER_H
