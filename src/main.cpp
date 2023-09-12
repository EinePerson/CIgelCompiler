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

std::string toAsm(const std::vector<Token>& tokens){
    std::stringstream output;
    output << "global _start\n_start:\n";
    for(int i = 0; i < tokens.size();i++){
        const Token& token = tokens.at(i);
        if(token.type == TokenType::_exit){
            if(i + 1 < tokens.size() && tokens.at(i + 1).type == TokenType::int_lit){
                if(i + 2 < tokens.size() && tokens.at(i + 2).type == TokenType::semi){
                    output << "     mov rax, 60\n" << "     mov rdi, " << tokens.at(i + 1).value.value() << "\n" << "   syscall";
                }
            }
        }
    }
    return output.str();
}

void write(std::string str,std::string name){
    std::fstream file("./build/" + name + ".asm",std::ios::out);
    file << str;
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

    std::fstream input(argv[1],std::ios::in);
    std::stringstream contStream;
    contStream << input.rdbuf();
    input.close();
    std::string cont = contStream.str();

    //std::vector<Token> tokens = tokenise(cont);
    /*for(Token token : tokens){
        std::string str;
        if(token.value)str = std::to_string(((int) token.type)) + ":" + token.value.value();
        else str = std::to_string(((int) token.type));
        std::cout << str <<  std::endl;
    }*/

    Tokenizer tokenizer(std::move(cont));
    std::vector<Token> tokens = tokenizer.tokenize();
    /*for(auto token : tokens){
        std::cout << isBinOp(token.type) << ":" << ((int) token.type) << "\n";
    }*/

    Parser parser(std::move(tokens));
    std::optional<NodeProgram> parsed = parser.parseProg();

    if(!parsed.has_value()){
        std::cerr << "Invalid Programm" << std::endl;
        exit(EXIT_FAILURE);
    }

    Generator gen(parsed.value());

    std::string name = removeExtension(argv[1]);
    write(gen.gen_prog(),name);

    std::string strs = "nasm -felf64 ./build/" + name + ".asm";
    const char* str = strs.c_str();
    system(str);
    std::string strs1 = "ld -o out.exe ./build/" + name + ".o";
    const char* str1 = strs1.c_str();
    system(str1);

    return EXIT_SUCCESS;
}