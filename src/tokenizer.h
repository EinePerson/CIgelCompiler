#pragma once

#include <fstream>
#include <string>
#include <utility>
#include <vector>
#include <optional>
#include <map>
#include <iostream>
#include <filesystem>
#include <set>
#include <llvm/IR/DerivedTypes.h>

typedef unsigned int uint;

enum class TokenType{
    uninit = -1,
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
    _bool = 10,

    ///Double equal sign '=='
    equal = 11,
    notequal = 12,
    bigequal = 13,
    smallequal = 14,
    big = 15,
    small = 16,

    ///Single equal sign '='
    eq = 17,
    plus_eq = 18,
    sub_eq = 19,
    div_eq = 20,
    mul_eq = 21,
    pow_eq = 22,
    inc = 23,
    dec = 24,
    _bitOr = 25,
    _bitAnd = 26,
    _not,
    str,
    _char_lit,
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
    _for,
    _while,
    _break,
    _continue,
    _switch,
    _case,
    _default,
    //: for switch
    next,

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
    ///Double Connector ::
    dConnect,

    _struct,
    _class,
    interface,
    _namespace,
    _enum,

    extends,
    implements,
    _abstract,
    _override,
    instanceOf,

    null,
    _static,
    final,

    _try,
    _catch,
    _throw,

    ///\brief used for inline if statement
    question,
};

std::optional<int> prec(TokenType type);

struct Position {
    uint line = -1;
    uint _char = -1;
    std::string file;
};

struct Token : Position{
    Token():type(TokenType::uninit){
        line = -1;
        _char = -1;
    }
    Token(const TokenType type, const uint line,const uint _char,std::string file,std::string value = ""): type(type), value(std::move(value)) {
        this->line = line;
        this->_char = _char;
        this->file = std::move(file);
    }
    Token(const Token& other)  : Position(other) {
        type = other.type;
        value = other.value;
        line = other.line;
        file = other.file;
        _char = other._char;
    }

    TokenType type;
    std::optional<std::string> value;

    Token& operator=(Token other){
        if (this == &other)return *this;
        this->type = other.type;
        this->value = other.value;
        this->line = other.line;
        this->file = other.file;
        this->_char = other._char;
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
            {"boolean",TokenType::_bool},
            {"bool",TokenType::_bool},

            {"==",TokenType::equal},
            {"!=",TokenType::notequal},
            {">=",TokenType::bigequal},
            {"<=",TokenType::smallequal},

            {"&&",TokenType::_and},
            {"||",TokenType::_or},
            {"!|",TokenType::_xor},

            {"else",TokenType::_else},
            {"break",TokenType::_break},
            {"continue",TokenType::_continue},
            {"default",TokenType::_default},
            {"case",TokenType::_case},
            {"::",TokenType::dConnect},

            {"void",TokenType::_void},
            {"return",TokenType::_return},

            {"use",TokenType::use},
            {"using",TokenType::use},
            {"include",TokenType::include},

            {"public",TokenType::_public},
            {"private",TokenType::_private},
            {"protected",TokenType::_protected},

            {"new",TokenType::_new},
            {"null",TokenType::null},

            {"struct",TokenType::_struct},
            {"class",TokenType::_class},
            {"namespace",TokenType::_namespace},
            {"enum",TokenType::_enum},
            {"interface",TokenType::interface},

            {"abstract",TokenType::_abstract},
            {"extends",TokenType::extends},
            {"implements",TokenType::implements},
            {"override",TokenType::_override},
            //{"super",TokenType::super},
            {"instanceOf",TokenType::instanceOf},

            {"static",TokenType::_static},
            {"final",TokenType::final},

            {"try",TokenType::_try},
            {"catch",TokenType::_catch},
            {"throw",TokenType::_throw},
            {"if",TokenType::_if},
            {"for",TokenType::_for},
            {"while",TokenType::_while},
            {"switch",TokenType::_switch}
    };

    const std::map<std::string,Token> REPLACE = {
            {"false",Token(TokenType::int_lit,(uint) -1,-1,"","0Z")},
            {"true",Token(TokenType::int_lit,(uint) -1,-1,"","1Z")},
    };

   const std::map<char,TokenType> IGEL_TOKEN_CHAR = {
            {';',TokenType::semi},
            {',',TokenType::comma},

            {'(',TokenType::openParenth},
            {')',TokenType::closeParenth},
            {'{',TokenType::openCurl},
            {'}',TokenType::closeCurl},
            {'[',TokenType::openBracket},
            {']',TokenType::closeBracket},

            {'=',TokenType::eq},
            {'+',TokenType::plus},
            {'-',TokenType::sub},
            {'/',TokenType::div},
            {'*',TokenType::mul},
            {'^',TokenType::pow},


            {'>',TokenType::big},
            {'<',TokenType::small},

            {'.',TokenType::connector},
            {':',TokenType::next},
    };

    const std::map<char,std::string> ESCAPES = {
            {'b',"\b"},
            {'t',"\t"},
            {'n',"\n"},
            {'v'," \v"},
            {'f',"\f"},
            {'r',"\r"},
            {'"',"\""},
    };

    static std::map<std::string,std::function<llvm::FunctionCallee()>> LIB_FUNCS;

   std::vector<Token> tokenize(const std::string&file);

private:
        [[nodiscard]] std::optional<char> peak(uint count = 0) {
            if(m_I + count >= m_src.length())return {};
            return m_src.at(m_I + count);
        };

        char consume(){
            charCount++;
            if(m_src[m_I] == '\n')lineCount++;
            return m_src.at(m_I++);
        }

        void err(const std::string&err) const {
            std::cerr << err << "\n" << "   at: " <<  m_tokens.back().file << ":" << m_tokens.back().line << ":" << m_tokens.back()._char << std::endl;
            exit(EXIT_FAILURE);
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

        bool isNum() {
            return peak().has_value() && (std::isdigit(peak().value()) || peak().value() == '.' || peak().value() == 'x' || peak().value() == 'b' || peak().value() == 'A' || peak().value() == 'B' || peak().value() == 'C'
                || peak().value() == 'D' || peak().value() == 'E' || peak().value() == 'F' || peak().value() == 'a' || peak().value() == 'c' || peak().value() == 'd' || peak().value() == 'e' || peak().value() == 'f' );
        }

        std::string m_src;
        size_t m_I = 0;
        std::set<std::string> m_extended_funcs;
        std::vector<Token> m_tokens {};
        uint charCount = 0;
        uint lineCount = 1;
};