//
// Created by igel on 11.11.23.
//

#ifndef IGEL_COMPILER_OPTIONSPARSER_H
#define IGEL_COMPILER_OPTIONSPARSER_H


#include <string>
#include <vector>
#include "InfoParser.h"

struct Options{
    std::optional<std::string> infoFile;
};

class OptionsParser {
public:

    OptionsParser(int argc,char* argv[]);

    Options* getOptions();

    void modify(Info* info);

private:
    std::vector<std::string> m_options;
    std::vector<std::string> m_files;
};


#endif //IGEL_COMPILER_OPTIONSPARSER_H
