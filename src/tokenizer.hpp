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

    //Double equal sign '=='
    equal = 8,
    notequal = 9,
    bigequal = 10,
    smallequal = 11,
    big = 12,
    small = 13,

    //Single equal sign '='
    eq = 14,
    plus_eq = 15,
    sub_eq = 16,
    div_eq = 17,
    mul_eq = 18,
    pow_eq = 19,
    int_lit,
    id,

    plus,
    sub,
    div,
    mul,
    pow,

    _and,
    _or,
    _xor,

    _exit,
    _if,
    _else,

    semi,
    openParenth,
    closeParenth,
    openCurl,
    closeCurl,
    
    
};

std::optional<int> prec(TokenType type){
    switch (type)
    {
    case TokenType::plus:
    case TokenType::sub:
        return 1;
    case TokenType::div:
    case TokenType::mul:
        return 2;
    case TokenType::pow:
        return 3;
    case TokenType::bigequal:
    case TokenType::equal:
    case TokenType::notequal:
    case TokenType::smallequal:
    case TokenType::small:
    case TokenType::big:
        return 0;
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
            //{"==",TokenType::equal},
            {"!=",TokenType::notequal},
            {">=",TokenType::bigequal},
            {"<=",TokenType::smallequal},
            {"else",TokenType::_else},
        };

        const std::map<std::string,TokenType> FUNCTIONS = {
            {"exit",TokenType::_exit},
            {"if",TokenType::_if},
        };

        const std::map<std::string,Token> REPLACE = {
            {"false",Token{.type = TokenType::int_lit,.value = "0"}},
            {"true",Token{.type = TokenType::int_lit,.value = "1"}}
        };

        const std::map<char,TokenType> IGEL_TOKEN_CHAR = {
            {';',TokenType::semi},
            {'(',TokenType::openParenth},
            {')',TokenType::closeParenth},
            //{'=',TokenType::eq},
            {'+',TokenType::plus},
            {'-',TokenType::sub},
            {'/',TokenType::div},
            {'*',TokenType::mul},
            {'^',TokenType::pow},
            {'{',TokenType::openCurl},
            {'}',TokenType::closeCurl},
            {'}',TokenType::closeCurl},
            {'>',TokenType::big},
            {'<',TokenType::small},
        };

        inline explicit Tokenizer(const std::string& src): m_src(src){

        }

        inline std::vector<Token> tokenize(){
            std::vector<Token> tokens {};
            std::string buf;

            char comment = 0;
            while (peak().has_value())
            {
                if(comment){
                    if(peak().value() == '\n' && comment == 1){
                        consume();
                        comment = false;
                    }else if(peak().value() == '*' && peak(1).has_value() && peak(1).value() == '/' && comment == 2){
                        consume();
                        consume();
                        comment = false;
                    }else consume();
                    continue;
                }
                if(peak().value() == '/'){
                    if(peak(1).has_value() && peak(1).value() == '/'){
                        consume();
                        comment = 1;
                        continue;
                    }else if(peak(1).has_value() && peak(1).value() == '*'){
                        consume();
                        comment = 2;
                        continue;
                    }else if(peak(1).has_value() && peak(1).value() == '='){
                        tokens.push_back({.type = TokenType::div_eq});
                        consume();
                        consume();
                        continue;
                    }else {
                        tokens.push_back({.type = TokenType::div});
                        consume();
                        continue;
                    }
                }else if(peak().value() == '!'){
                    consume();
                    if(peak().has_value() && peak().value() == '='){
                        tokens.push_back({.type = TokenType::notequal});
                        consume();
                        continue;
                    }else if(peak().has_value() && peak().value() == '|'){
                        tokens.push_back({.type = TokenType::_xor});
                        consume();
                        continue;
                    }else{
                        std::cerr << "Expected '='1" <<  std::endl;
                        exit(EXIT_FAILURE);
                    }
                }else if(peak().value() == '>'){
                    consume();
                    if(peak().has_value() && peak().value() == '='){
                        tokens.push_back({.type = TokenType::bigequal});
                        consume();
                        continue;
                    }else{
                        tokens.push_back({.type = TokenType::big});
                        continue;
                    }
                }else if(peak().value() == '<'){
                    consume();
                    if(peak().has_value() && peak().value() == '='){
                        tokens.push_back({.type = TokenType::smallequal});
                        consume();
                        continue;
                    }else{
                        tokens.push_back({.type = TokenType::small});
                        continue;
                    }
                }else if(peak().value() == '='){
                    consume();
                    if(peak().has_value() && peak().value() == '='){
                        tokens.push_back({.type = TokenType::equal});
                        consume();
                        continue;
                    }else {
                        tokens.push_back({.type = TokenType::eq});
                        consume();
                        continue;
                    }
                }else if(peak().value() == '&'){
                    consume();
                    if(peak().has_value() && peak().value() == '&'){
                        tokens.push_back({.type = TokenType::_and});
                        consume();
                        continue;
                    }else{
                        std::cerr << "Expected '&'" <<  std::endl;
                        exit(EXIT_FAILURE);
                    }
                }else if(peak().value() == '|'){
                    consume();
                    if(peak().has_value() && peak().value() == '|'){
                        tokens.push_back({.type = TokenType::_or});
                        consume();
                        continue;
                    }else{
                        std::cerr << "Expected '|'" <<  std::endl;
                        exit(EXIT_FAILURE);
                    }
                }else if(peak().value() == '+'){
                    consume();
                    if(peak().has_value() && peak().value() == '='){
                        tokens.push_back({.type = TokenType::plus_eq});
                        consume();
                        continue;
                    }else {
                        tokens.push_back({.type = TokenType::plus});
                        consume();
                        continue;
                    }
                }else if(peak().value() == '-'){
                    consume();
                    if(peak().has_value() && peak().value() == '='){
                        tokens.push_back({.type = TokenType::sub_eq});
                        consume();
                        continue;
                    }else {
                        tokens.push_back({.type = TokenType::sub});
                        consume();
                        continue;
                    }
                }else if(peak().value() == '*'){
                    consume();
                    if(peak().has_value() && peak().value() == '='){
                        tokens.push_back({.type = TokenType::mul_eq});
                        consume();
                        continue;
                    }else {
                        tokens.push_back({.type = TokenType::mul});
                        consume();
                        continue;
                    }
                }else if(peak().value() == '/'){
                    consume();
                    if(peak().has_value() && peak().value() == '='){
                        tokens.push_back({.type = TokenType::div_eq});
                        consume();
                        continue;
                    }else {
                        tokens.push_back({.type = TokenType::div});
                        consume();
                        continue;
                    }
                }else if(peak().value() == '*'){
                    consume();
                    if(peak().has_value() && peak().value() == '='){
                        tokens.push_back({.type = TokenType::pow_eq});
                        consume();
                        continue;
                    }else {
                        tokens.push_back({.type = TokenType::pow});
                        consume();
                        continue;
                    }
                }else if(std::isalpha(peak().value())){
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
                    }else if(REPLACE.count(buf)){
                        tokens.push_back(REPLACE.at(buf));
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
                    if(peak().has_value() && std::isalpha(peak().value()))buf.push_back(consume());
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
            
            if(comment == 2){
                std::cerr << "Unclosed comment" << std::endl;
                exit(EXIT_FAILURE);
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