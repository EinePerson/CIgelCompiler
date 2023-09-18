#pragma once

#include <string>
#include <vector>
#include <optional>
#include <map>
#include <iostream>

typedef unsigned int uint;

enum class TokenType{
    _byte = 0,
    _short = 1,
    _int = 2,
    _long = 3,
    _ubyte = 4,
    _ushort = 5,
    _uint = 6,
    _ulong = 7,
    plus,
    sub,
    div,
    mul,
    pow,
    _exit,
    int_lit,
    semi,
    openParenth,
    closeParenth,
    openCurl,
    closeCurl,
    id,
    eq,
    _if,
};

std::optional<int> prec(TokenType type){
    switch (type)
    {
    case TokenType::plus:
    case TokenType::sub:
        return 0;
    case TokenType::div:
    case TokenType::mul:
        return 1;
    case TokenType::pow:
        return 2;
    
    default:
        return {};
    }
}

struct Token{
    TokenType type;
    std::optional<std::string> value;
};

class Tokenizer{
    public:
        const std::map<std::string,TokenType> IGEL_TOKENS = {
            {"byte",TokenType::_byte},
            {"short",TokenType::_short},
            {"int",TokenType::_int},
            {"long",TokenType::_long},
            {"ubyte",TokenType::_ubyte},
            {"ushort",TokenType::_ushort},
            {"uint",TokenType::_uint},
            {"ulong",TokenType::_ulong},
        };

        const std::map<std::string,TokenType> FUNCTIONS = {
            {"exit",TokenType::_exit},
            {"if",TokenType::_if},
        };

        const std::map<char,TokenType> IGEL_TOKEN_CHAR = {
            {';',TokenType::semi},
            {'(',TokenType::openParenth},
            {')',TokenType::closeParenth},
            {'=',TokenType::eq},
            {'+',TokenType::plus},
            {'-',TokenType::sub},
            {'/',TokenType::div},
            {'*',TokenType::mul},
            {'^',TokenType::pow},
            {'{',TokenType::openCurl},
            {'}',TokenType::closeCurl},
        };

        inline explicit Tokenizer(const std::string& src): m_src(src){

        }

        inline std::vector<Token> tokenize(){
            std::vector<Token> tokens {};
            std::string buf;

            while (peak().has_value())
            {
                if(std::isalpha(peak().value())){
                    buf.push_back(consume());
                    while (peak().has_value() && std::isalnum(peak().value()))
                    {
                        buf.push_back(consume());
                    }
                    while(peak().has_value() && std::isspace(peak().value())){
                        consume();
                        continue;
                    }
                    if(peak().has_value() && peak().value() == '('){
                        if(FUNCTIONS.count(buf)){
                            tokens.push_back({.type = FUNCTIONS.at(buf)});
                            buf.clear();
                            continue;
                        }else {
                            std::cerr << "Unkown function reference " << buf << std::endl;
                            exit(EXIT_FAILURE);
                        }
                    }else if(IGEL_TOKENS.count(buf)){
                        tokens.push_back({.type = IGEL_TOKENS.at(buf)});
                        buf.clear();
                        continue;
                    }else {
                        tokens.push_back({.type = TokenType::id,.value = buf});
                        buf.clear();
                        continue;
                    }
                }else if(std::isdigit(peak().value())){
                    buf.push_back(consume());
                    while (peak().has_value() && std::isdigit(peak().value()))
                    {
                        buf.push_back(consume());
                    }
                    tokens.push_back({.type = TokenType::int_lit,.value = buf});
                    buf.clear();
                    continue;
                }else if(IGEL_TOKEN_CHAR.count(peak().value())){
                    tokens.push_back({.type = IGEL_TOKEN_CHAR.at(consume())});
                    continue;
                }else if(std::isspace(peak().value())){
                    consume();
                    continue;
                }else {
                    std::cerr << "Unkown Symbol " << consume() << std::endl;
                    exit(EXIT_FAILURE);
                }
            }
            
            m_I = 0;
            return tokens;
        }

    private:
        inline std::optional<char> peak(uint count = 0) const{
            if(m_I + count >= m_src.length())return {};
            else return m_src.at(m_I + count);
        };

        inline char consume(){
            return m_src.at(m_I++);
        }

        const std::string m_src;
        size_t m_I = 0;
};