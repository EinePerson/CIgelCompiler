#pragma once

#include <fstream>
#include <string>
#include <utility>
#include <vector>
#include <optional>
#include <map>
#include <iostream>
#include <filesystem>
#include <functional>
#include <set>

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
    _float = 8,
    _double = 9,

    ///Double equal sign '=='
    equal = 10,
    notequal = 11,
    bigequal = 12,
    smallequal = 13,
    big = 14,
    small = 15,

    ///Single equal sign '='
    eq = 16,
    plus_eq = 17,
    sub_eq = 18,
    div_eq = 19,
    mul_eq = 20,
    pow_eq = 21,
    inc = 22,
    dec = 23,
    str,
    int_lit,
    id,

    _new,

    plus,
    sub,
    div,
    mul,
    pow,

    _and,
    _or,
    ///Not implemented and not directly planed to do so
    _xor,

    ///DEPRECATED
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
    include,

    _public,
    _private,
    _protected,

    externFunc,
    info,

    ///Dot to acces vars of classes/structs
    connector,
    _struct,
    null,
};

std::optional<int> prec(TokenType type);

struct Token;

struct Token{
    TokenType type;
    std::optional<std::string> value {};
    uint line = -1;
    std::string file;

    Token& operator=(Token other){
        if (this == &other)return *this;
        this->type = other.type;
        this->value = other.value;
        this->line = other.line;
        this->file = other.file;
        return *this;
    }
};

class Tokenizer{
    public:

    explicit Tokenizer(const std::set<std::string>& extended_funcs = {});

    const std::map<std::string,TokenType> IGEL_TOKENS = {
            {"byte",TokenType::_byte},
            {"short",TokenType::_short},
            {"int",TokenType::_int},
            {"long",TokenType::_long},
            {"ubyte",TokenType::_ubyte},
            {"ushort",TokenType::_ushort},
            {"uint",TokenType::_uint},
            {"ulong",TokenType::_ulong},
            {"float",TokenType::_float},
            {"double",TokenType::_double},
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
            {"include",TokenType::include},
            {"public",TokenType::_public},
            {"private",TokenType::_private},
            {"protected",TokenType::_protected},
            {"new",TokenType::_new},
            {"struct",TokenType::_struct},
            {"null",TokenType::null},
    };

    const std::map<std::string,TokenType> FUNCTIONS = {
            {"exit",TokenType::_exit},
            {"if",TokenType::_if},
    };

    Token token{.type = TokenType::int_lit};

    const std::map<std::string,Token> REPLACE = {
            {"false",Token{.type = TokenType::int_lit}},
            {"true",Token{.type = TokenType::int_lit}}
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
            {'.',TokenType::connector},
    };

   //std::vector<Token> tokenize(std::string file);


   std::vector<Token> tokenize(std::string file);

private:
        [[nodiscard]] std::optional<char> peak(uint count = 0) const{
            if(m_I + count >= m_src.length())return {};
            else return m_src.at(m_I + count);
        };

        char consume(){
            return m_src.at(m_I++);
        }

        static std::string read(const std::string& name){
            std::ifstream file(name,std::ios::in);
            if(!file.is_open()){
                std::cerr << "Could not open: " << name << std::endl;
                exit(EXIT_FAILURE);
            }
            std::stringstream r;
            r << file.rdbuf();
            file.close();
            return r.str();
        }

        std::string m_src;
        size_t m_I = 0;
        std::set<std::string> m_extended_funcs;
};