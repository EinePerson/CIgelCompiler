#include <iostream>
#include <fstream>
#include <sstream>

#include "langMain/langMain.hpp"
#include "CompilerInfo/CompilerInfo.hpp"
#include "CompilerInfo/OptionsParser.h"
#include "cxx_extension/CXX_Parser.h"

typedef unsigned int uint;

void write(const std::string& str,const std::string& name){
    std::fstream file(name,std::ios::out);
    file << str;
    file.close();
}

std::string read(const std::string& name){
    std::ifstream file(name,std::ios::in);
    std::stringstream r;
    r << file.rdbuf();
    file.close();
    return r.str();
}

int main(int argc,char* argv[]) {
    OptionsParser optParser(argc,argv);
    Options* options = optParser.getOptions();

    Info *info = nullptr;
    if(options->infoFile) {
        CompilerInfo cmpinfo(options->infoFile.value());
        info = cmpinfo.getInfo();
    }else {
        std::cerr << "Currently a build file is needed" << std::endl;
        exit(EXIT_FAILURE);
    }
    optParser.modify(info);
    if(options->infoFile) {
        std::string name = options->infoFile.value();
        auto pos = name.find_last_of('/');
        if(pos != std::string::npos)name.erase(pos);
        info->execDir = name;
    }

    LangMain main(info,options);
    main.compile();

    return EXIT_SUCCESS;
}