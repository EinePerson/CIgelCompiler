#pragma once

#include <string>
#include <utility>
#include <vector>
#include <map>

#include "InfoParser.h"


class CompilerInfo{

    public:
        CompilerInfo(std::string file) : m_file(std::move(file)),m_tokenizer({"setMain","SourceDirectory","setOutName"}) {
        }

        Info* getInfo(){
            std::vector<Token> tokens = m_tokenizer.tokenize(m_file);
            /*if(!tokens.size()){
                std::cout << "Unable to tokenise Compiler info" << std::endl;
                exit(EXIT_FAILURE);
            }*/
    	    InfoParser parser(tokens);
            Info* info = parser.parse(); 
            return info;
        }

    private:
        std::string m_file;
        Tokenizer m_tokenizer;
};