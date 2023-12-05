#include <iostream>
#include <fstream>
#include <sstream>
#include <cstring>

#include "langMain/langMain.hpp"
#include "CompilerInfo/CompilerInfo.hpp"
#include "CompilerInfo/OptionsParser.h"

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
    if(argc < 2){
        std::cerr << "Incorrect arguments, type -help for more information" << std::endl;
        return EXIT_FAILURE;
    }

    if(strcmp(argv[1], "help" ) == 0 ){
        std::cout << "Help:\n   First arg should be igel Compiler file(.igc)\n";
        return EXIT_SUCCESS;
    }
    OptionsParser optParser(argc,argv);
    Options* options = optParser.getOptions();

    Info *info;
    if(options->infoFile) {
        CompilerInfo cmpinfo(options->infoFile.value());
        info = cmpinfo.getInfo();
    }
    optParser.modify(info);

    LangMain main(info,options);
    main.compile();

    return EXIT_SUCCESS;
}