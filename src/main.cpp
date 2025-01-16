#include <iostream>
#include <fstream>
#include <sstream>

#include "SelfInfo.h"
#include "langMain/langMain.hpp"
#include "CompilerInfo/OptionsParser.h"
#include "cxx_extension/CXX_Parser.h"
#include "util/Mangler.h"

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
    Igel::Mangler::init(Igel::ITANIUM);
    OptionsParser optParser(argc,argv);
    Options* options = optParser.getOptions();

    InternalInfo *info = options->info;
    /*if(options->infoFile) {
        info = cmpinfo.getInfo();
    }else {
        std::cerr << "Currently a build file is needed" << std::endl;
        exit(EXIT_FAILURE);
    }*/
    if(options->infoFile) {
        std::string name = options->infoFile.value();
        auto pos = name.find_last_of('/');
        if(pos != std::string::npos)name.erase(pos);
        //info->execDir = name;
    }


    LangMain main(info,options);
    main.compile();
    main.exec();

    return EXIT_SUCCESS;
}