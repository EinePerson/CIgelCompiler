#include <iostream>
#include <fstream>
#include <sstream>
#include <optional>
#include <vector>
#include <map>

#include "tokenizer.hpp"
#include "parser.hpp"
#include "generator.hpp"
#include "arena.hpp"

void write(std::string str,std::string name){
    std::fstream file(name,std::ios::out);
    file << str;
    file.close();
}

std::string read(std::string name){
    std::ifstream file(name,std::ios::in);
    std::stringstream r;
    r << file.rdbuf();
    file.close();
    return r.str();
}

std::string removeExtension(const std::string& filename) {
    size_t lastdot = filename.find_last_of(".");
    if (lastdot == std::string::npos) return filename;
    return filename.substr(0, lastdot); 
}

int main(int argc,char* argv[]) {
    if(argc != 2){
        std::cerr << "Incorrect arguments: expected an Igel source file as second argument" << std::endl;
        return EXIT_FAILURE;
    }

    std::string name = removeExtension(argv[1]);
    std::string cont = read(argv[1]);
    


    Tokenizer tokenizer(std::move(cont));
    std::vector<Token> tokens = tokenizer.tokenize();
    Parser parser(std::move(tokens));
    std::optional<NodeProgram> parsed = parser.parseProg();
    if(!parsed.has_value()){
        std::cerr << "Invalid Programm" << std::endl;
        exit(EXIT_FAILURE);
    }
    Generator gen(parsed.value());
    //write(read("./build/" + name + ".asm"),"./build/" + name + "-old.asm");
    write(gen.gen_prog(),"./build/" + name + ".asm");
    std::string strs = "nasm -felf64 ./build/" + name + ".asm";
    const char* str = strs.c_str();
    system(str);
    std::string strs1 = "ld -o out.exe ./build/" + name + ".o";
    const char* str1 = strs1.c_str();
    system(str1);

    return EXIT_SUCCESS;
}