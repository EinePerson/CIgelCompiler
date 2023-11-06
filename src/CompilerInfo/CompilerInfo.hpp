#pragma once

#include <string>
#include <utility>
#include <vector>
#include <map>

#include "../util/arena.hpp"
//#include "../tokenizer.h"
#include "InfoParser.h"


class CompilerInfo{

    public:
        CompilerInfo(std::string file,ArenaAlocator* alloc) : m_file(std::move(file)),m_tokenizer({"setMain","setSourceDirectory","setOutName"}) {
            m_alloc = alloc;
        }

        Info* getInfo(){
            std::vector<Token> tokens = m_tokenizer.tokenize(m_file);
            /*if(!tokens.size()){
                std::cout << "Unable to tokenise Compiler info" << std::endl;
                exit(EXIT_FAILURE);
            }*/
    	    InfoParser parser(tokens,m_alloc);
            Info* info = parser.parse(); 
            return info;
        }

    private:
        ArenaAlocator* m_alloc;
        std::string m_file;
        Tokenizer m_tokenizer;
};