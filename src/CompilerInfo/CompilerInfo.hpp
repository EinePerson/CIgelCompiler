#pragma once

#include <string>
#include <utility>
#include <vector>
#include <map>
#include <ranges>

#include "InfoParser.h"


class CompilerInfo{

    public:
    explicit CompilerInfo(std::string file) : m_file(std::move(file)),m_tokenizer({std::views::keys(InfoParser::funcs).begin(),std::views::keys(InfoParser::funcs).end()}) {
        }

        Info* getInfo(){
            std::vector<Token> tokens = m_tokenizer.tokenize(m_file);
    	    InfoParser parser(tokens);
            Info* info = parser.parse(); 
            return info;
        }

    private:
        std::string m_file;
        Tokenizer m_tokenizer;
};