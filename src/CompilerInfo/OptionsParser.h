//
// Created by igel on 11.11.23.
//

#ifndef IGEL_COMPILER_OPTIONSPARSER_H
#define IGEL_COMPILER_OPTIONSPARSER_H


#include <string>
#include <vector>
#include "../util/arena.hpp"
#include "InfoParser.h"

struct Options{
    std::optional<std::string> infoFile;
};

class OptionsParser {
public:

    OptionsParser(int argc,char* argv[],ArenaAlocator* alloc);

    Options* getOptions();

    void modify(Info* info);

private:
    std::vector<std::string> m_options;
    std::vector<std::string> m_files;
    //std::vector<SrcFile*> m_srcFiles;
    ArenaAlocator* m_alloc;
};


#endif //IGEL_COMPILER_OPTIONSPARSER_H
