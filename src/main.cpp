#include <iostream>
#include <fstream>
#include <sstream>
#include <cstring>

#include "langMain/langMain.hpp"
#include "util/arena.hpp"
#include "CompilerInfo/CompilerInfo.hpp"

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
        std::cerr << "Incorrect arguments: expected an Igel source file as second argument" << std::endl;
        return EXIT_FAILURE;
    }   

    if(strcmp(argv[1], "help" ) == 0 ){
        std::cout << "Help:\n   First arg should be igel Compiler file(.igc)\n";
        return EXIT_SUCCESS;
    }

    ArenaAlocator alloc(1024 * 1024 * 4);
    CompilerInfo cmpinfo(argv[1],&alloc);
    Info* info = cmpinfo.getInfo();
    LangMain main(info);
    main.compile();

    /*std::string name = removeExtension(argv[1]);
    std::string cont = read(argv[1]);
    
    Tokenizer tokenizer(std::move(cont));
    std::vector<Token> tokens = tokenizer.tokenize();

    if(!tokens.size()){
        std::cerr << "Invalid Programm" << std::endl;
        exit(EXIT_FAILURE);
    }

    Parser InfoParser(std::move(tokens));
    std::optional<NodeProgram> parsed = InfoParser.parseProg();

    if(!parsed.has_value()){
        std::cerr << "Invalid Programm" << std::endl;
        exit(EXIT_FAILURE);
    }

    Generator gen(parsed.value());

    std::string prog = gen.gen_prog();
    write(prog,"./build/" + name + ".asm");
    std::string strs = "nasm -felf64 ./build/" + name + ".asm";
    const char* str = strs.c_str();
    system(str);
    std::string strs1 = "ld -o out.exe ./build/" + name + ".o";
    const char* str1 = strs1.c_str();
    system(str1);*/

    return EXIT_SUCCESS;
}

//#include "tokenizer.cpp"