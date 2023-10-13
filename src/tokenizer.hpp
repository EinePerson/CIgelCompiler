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
    inc = 20,
    dec = 21,
    int_lit,
    id,

    plus,
    sub,
    div,
    mul,
    pow,

    _and,
    _or,
    //Not implemented and not directly planed to do so
    _xor,

    //DEPRECATED
    _exit,
    _if,
    _else,

    semi,
    comma,
    openParenth,
    closeParenth,
    openCurl,
    closeCurl,
    openBracket,
    closeBracket,
    
    _void,
    _return,

    use,
    grab,
};

std::optional<int> prec(TokenType type){
    switch (type)
    {
    case TokenType::bigequal:
    case TokenType::equal:
    case TokenType::notequal:
    case TokenType::smallequal:
    case TokenType::small:
    case TokenType::big:
        return 0;    
    case TokenType::plus:
    case TokenType::sub:
        return 1;
    case TokenType::div:
    case TokenType::mul:
        return 2;
    case TokenType::pow:
        return 3;
    default:
        return {};
    }
}

struct Token{
    TokenType type;
    std::optional<std::string> value;
    uint line;
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
            {"==",TokenType::equal},
            {"!=",TokenType::notequal},
            {">=",TokenType::bigequal},
            {"<=",TokenType::smallequal},
            {"&&",TokenType::_and},
            {"||",TokenType::_or},
            {"!|",TokenType::_xor},
            {"else",TokenType::_else},
            {"void",TokenType::_void},
            {"return",TokenType::_return},
            {"use",TokenType::use},
            {"using",TokenType::use},
            {"grab",TokenType::grab},
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
            {',',TokenType::comma},
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
            {'}',TokenType::closeCurl},
            {'>',TokenType::big},
            {'<',TokenType::small},
            {'[',TokenType::openBracket},
            {']',TokenType::closeBracket},
        };

        inline explicit Tokenizer(const std::string& src): m_src(src){

        }

        inline std::vector<Token> tokenize(){
            std::vector<Token> tokens {};
            std::string buf;

            char comment = 0;
            uint lineCount = 1;
            while (peak().has_value())
            {
                if(peak().value() == '\n')lineCount++;
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
                        tokens.push_back({.type = TokenType::div_eq,.line = lineCount});
                        consume();
                        consume();
                        continue;
                    }else {
                        tokens.push_back({.type = TokenType::div,.line = lineCount});
                        consume();
                        continue;
                    }
                }else if(peak().value() == '!'){
                    consume();
                    if(peak().has_value() && peak().value() == '='){
                        tokens.push_back({.type = TokenType::notequal,.line = lineCount});
                        consume();
                        continue;
                    }else if(peak().has_value() && peak().value() == '|'){
                        tokens.push_back({.type = TokenType::_xor,.line = lineCount});
                        consume();
                        continue;
                    }else{
                        std::cerr << "Expected '='1" <<  std::endl;
                        exit(EXIT_FAILURE);
                    }
                }else if(peak().value() == '>'){
                    consume();
                    if(peak().has_value() && peak().value() == '='){
                        tokens.push_back({.type = TokenType::bigequal,.line = lineCount});
                        consume();
                        continue;
                    }else{
                        tokens.push_back({.type = TokenType::big,.line = lineCount});
                        continue;
                    }
                }else if(peak().value() == '<'){
                    consume();
                    if(peak().has_value() && peak().value() == '='){
                        tokens.push_back({.type = TokenType::smallequal,.line = lineCount});
                        consume();
                        continue;
                    }else{
                        tokens.push_back({.type = TokenType::small,.line = lineCount});
                        continue;
                    }
                }else if(peak().value() == '='){
                    consume();
                    if(peak().has_value() && peak().value() == '='){
                        tokens.push_back({.type = TokenType::equal,.line = lineCount});
                        consume();
                        continue;
                    }else {
                        tokens.push_back({.type = TokenType::eq,.line = lineCount});
                        consume();
                        continue;
                    }
                }else if(peak().value() == '&'){
                    consume();
                    if(peak().has_value() && peak().value() == '&'){
                        tokens.push_back({.type = TokenType::_and,.line = lineCount});
                        consume();
                        continue;
                    }else{
                        std::cerr << "Expected '&'" <<  std::endl;
                        exit(EXIT_FAILURE);
                    }
                }else if(peak().value() == '|'){
                    consume();
                    if(peak().has_value() && peak().value() == '|'){
                        tokens.push_back({.type = TokenType::_or,.line = lineCount});
                        consume();
                        continue;
                    }else{
                        std::cerr << "Expected '|'" <<  std::endl;
                        exit(EXIT_FAILURE);
                    }
                }else if(peak().value() == '+'){
                    consume();
                    if(peak().has_value() && peak().value() == '='){
                        tokens.push_back({.type = TokenType::plus_eq,.line = lineCount});
                        consume();
                        continue;
                    }else if(peak().has_value() && peak().value() == '+'){
                        tokens.push_back({.type = TokenType::inc,.line = lineCount});
                        consume();
                        continue;
                    }else {
                        tokens.push_back({.type = TokenType::plus,.line = lineCount});
                        consume();
                        continue;
                    }
                }else if(peak().value() == '-'){
                    consume();
                    if(peak().has_value() && peak().value() == '='){
                        tokens.push_back({.type = TokenType::sub_eq,.line = lineCount});
                        consume();
                        continue;
                    }else if(peak().has_value() && peak().value() == '-'){
                        tokens.push_back({.type = TokenType::dec,.line = lineCount});
                        consume();
                        continue;
                    }else {
                        tokens.push_back({.type = TokenType::sub,.line = lineCount});
                        consume();
                        continue;
                    }
                }else if(peak().value() == '*'){
                    consume();
                    if(peak().has_value() && peak().value() == '='){
                        tokens.push_back({.type = TokenType::mul_eq,.line = lineCount});
                        consume();
                        continue;
                    }else {
                        tokens.push_back({.type = TokenType::mul,.line = lineCount});
                        consume();
                        continue;
                    }
                }else if(peak().value() == '/'){
                    consume();
                    if(peak().has_value() && peak().value() == '='){
                        tokens.push_back({.type = TokenType::div_eq,.line = lineCount});
                        consume();
                        continue;
                    }else {
                        tokens.push_back({.type = TokenType::div,.line = lineCount});
                        consume();
                        continue;
                    }
                }else if(peak().value() == '*'){
                    consume();
                    if(peak().has_value() && peak().value() == '='){
                        tokens.push_back({.type = TokenType::pow_eq,.line = lineCount});
                        consume();
                        continue;
                    }else {
                        tokens.push_back({.type = TokenType::pow,.line = lineCount});
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
                            tokens.push_back({.type = TokenType::id,.value = buf,.line = lineCount});
                            buf.clear();
                            continue;
                        }
                    }else if(IGEL_TOKENS.count(buf)){
                        tokens.push_back({.type = IGEL_TOKENS.at(buf),.line = lineCount});
                        buf.clear();
                        continue;
                    }else if(REPLACE.count(buf)){
                        tokens.push_back(REPLACE.at(buf));
                        buf.clear();
                        continue;
                    }else {
                        tokens.push_back({.type = TokenType::id,.value = buf,.line = lineCount});
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
                    tokens.push_back({.type = TokenType::int_lit,.value = buf,.line = lineCount});
                    buf.clear();
                    continue;
                }else if(IGEL_TOKEN_CHAR.count(peak().value())){
                    tokens.push_back({.type = IGEL_TOKEN_CHAR.at(consume()),.line = lineCount});
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