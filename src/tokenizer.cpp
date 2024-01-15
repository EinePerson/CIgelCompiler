#include "tokenizer.h"

#include "langMain/codeGen/Generator.h"

typedef unsigned int uint;

    std::map<std::string,std::function<llvm::FunctionCallee()>> Tokenizer::LIB_FUNCS {
        {"exit",[]() -> FunctionCallee {
            std::vector<Type*> params = {Type::getInt32Ty(*Generator::instance->m_contxt)};
            FunctionType *fType = FunctionType::get(Type::getVoidTy(*Generator::instance->m_contxt), params, false);
            return Generator::instance->m_module->getOrInsertFunction("exit",fType);
        }},
        {"printf",[]() -> FunctionCallee {
            std::vector<Type*> params = {PointerType::get(*Generator::instance->m_contxt,0)};
            FunctionType *fType = FunctionType::get(Type::getVoidTy(*Generator::instance->m_contxt), params, true);
            return Generator::instance->m_module->getOrInsertFunction("printf",fType);
        }},
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

    Tokenizer:: Tokenizer(const std::set<std::string>& extended_funcs) : m_extended_funcs(extended_funcs){}

     std::vector<Token> Tokenizer::tokenize(const std::string&file){
        m_src = read(file);
        m_I = 0;
        m_tokens = {};
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
                    m_tokens.push_back({.type = TokenType::div_eq,.line = lineCount,.file = file});
                    consume();
                    consume();
                    continue;
                }else {
                    m_tokens.push_back({.type = TokenType::div,.line = lineCount,.file = file});
                    consume();
                    continue;
                }
            }else if(peak().value() == '!'){
                consume();
                if(peak().has_value() && peak().value() == '='){
                    m_tokens.push_back({.type = TokenType::notequal,.line = lineCount,.file = file});
                    consume();
                    continue;
                }else if(peak().has_value() && peak().value() == '|'){
                    m_tokens.push_back({.type = TokenType::_xor,.line = lineCount,.file = file});
                    consume();
                    continue;
                }else{
                    err("Expected '='1");
                }
            }else if(peak().value() == '>'){
                consume();
                if(peak().has_value() && peak().value() == '='){
                    m_tokens.push_back({.type = TokenType::bigequal,.line = lineCount,.file = file});
                    consume();
                    continue;
                }else{
                    m_tokens.push_back({.type = TokenType::big,.line = lineCount,.file = file});
                    continue;
                }
            }else if(peak().value() == '<'){
                consume();
                if(peak().has_value() && peak().value() == '='){
                    m_tokens.push_back({.type = TokenType::smallequal,.line = lineCount,.file = file});
                    consume();
                    continue;
                }else{
                    m_tokens.push_back({.type = TokenType::small,.line = lineCount,.file = file});
                    continue;
                }
            }else if(peak().value() == '='){
                consume();
                if(peak().has_value() && peak().value() == '='){
                    m_tokens.push_back({.type = TokenType::equal,.line = lineCount,.file = file});
                    consume();
                    continue;
                }else {
                    m_tokens.push_back({.type = TokenType::eq,.line = lineCount,.file = file});
                    consume();
                    continue;
                }
            }else if(peak().value() == '&'){
                consume();
                if(peak().has_value() && peak().value() == '&'){
                    m_tokens.push_back({.type = TokenType::_and,.line = lineCount,.file = file});
                    consume();
                    continue;
                }else{
                    err("Expected '&'");
                }
            }else if(peak().value() == '|'){
                consume();
                if(peak().has_value() && peak().value() == '|'){
                    m_tokens.push_back({.type = TokenType::_or,.line = lineCount,.file = file});
                    consume();
                    continue;
                }else{
                    err("Expected '|'");
                }
            }else if(peak().value() == '+'){
                consume();
                if(peak().has_value() && peak().value() == '='){
                    m_tokens.push_back({.type = TokenType::plus_eq,.line = lineCount,.file = file});
                    consume();
                    continue;
                }else if(peak().has_value() && peak().value() == '+'){
                    m_tokens.push_back({.type = TokenType::inc,.line = lineCount,.file = file});
                    consume();
                    continue;
                }else {
                    m_tokens.push_back({.type = TokenType::plus,.line = lineCount,.file = file});
                    consume();
                    continue;
                }
            }else if(peak().value() == '-'){
                consume();
                if(peak().has_value() && peak().value() == '='){
                    m_tokens.push_back({.type = TokenType::sub_eq,.line = lineCount,.file = file});
                    consume();
                    continue;
                }else if(peak().has_value() && peak().value() == '-'){
                    m_tokens.push_back({.type = TokenType::dec,.line = lineCount,.file = file});
                    consume();
                    continue;
                }else {
                    m_tokens.push_back({.type = TokenType::sub,.line = lineCount,.file = file});
                    consume();
                    continue;
                }
            }else if(peak().value() == '*'){
                consume();
                if(peak().has_value() && peak().value() == '='){
                    m_tokens.push_back({.type = TokenType::mul_eq,.line = lineCount,.file = file});
                    consume();
                    continue;
                }else {
                    m_tokens.push_back({.type = TokenType::mul,.line = lineCount,.file = file});
                    consume();
                    continue;
                }
            }else if(peak().value() == '/'){
                consume();
                if(peak().has_value() && peak().value() == '='){
                    m_tokens.push_back({.type = TokenType::div_eq,.line = lineCount,.file = file});
                    consume();
                    continue;
                }else {
                    m_tokens.push_back({.type = TokenType::div,.line = lineCount,.file = file});
                    consume();
                    continue;
                }
            }else if(peak().value() == '*'){
                consume();
                if(peak().has_value() && peak().value() == '='){
                    m_tokens.push_back({.type = TokenType::pow_eq,.line = lineCount,.file = file});
                    consume();
                    continue;
                }else {
                    m_tokens.push_back({.type = TokenType::pow,.line = lineCount,.file = file});
                    consume();
                    continue;
                }
            }else if(peak().value() == '"'){
                consume();
                while (peak().has_value() && peak().value() != '"')
                {
                    buf.push_back(consume());
                }
                consume();
                m_tokens.push_back({.type = TokenType::str,.value = buf,.line = lineCount,.file = file});
                buf.clear();
                continue;
            }else if(peak().value() == '\'') {
                consume();
                if(peak().has_value() && peak().value() != '\'')m_tokens.push_back({.type = TokenType::_char_lit,.value = std::string{consume()},.line = lineCount,.file = file});
                consume();
            }else if(peak().value() == '#'){
                consume();
                while (peak().has_value() && std::isalnum(peak().value()))
                {
                    buf.push_back(consume());
                }
                while(peak().has_value() && std::isspace(peak().value())){
                    consume();
                }
                m_tokens.push_back({.type = TokenType::info,.value = buf});
                buf.clear();
                continue;
            }else if(peak().value() == ':') {
                consume();
                if(peak().value() == ':') {
                    consume();
                    m_tokens.push_back({.type =  TokenType::dConnect,.line = lineCount,.file = file});
                    continue;
                }
                m_tokens.push_back({.type =  TokenType::next,.line = lineCount,.file = file});
                continue;
            }else if(std::isalpha(peak().value())){
                buf.push_back(consume());
                while (peak().has_value() && std::isalnum(peak().value()))
                {
                    buf.push_back(consume());
                }
                while(peak().has_value() && std::isspace(peak().value())){
                    consume();
                }
                if(peak().has_value() && peak().value() == '('){
                    if(FUNCTIONS.count(buf)){
                        m_tokens.push_back({.type = FUNCTIONS.at(buf)});
                        buf.clear();
                        continue;
                    }else if(m_extended_funcs.count(buf)){
                        m_tokens.push_back({.type = TokenType::externFunc,.value = buf,.line = lineCount,.file = file});
                        buf.clear();
                        continue;
                    }else{
                        m_tokens.push_back({.type = TokenType::id,.value = buf,.line = lineCount,.file = file});
                        buf.clear();
                        continue;
                    }
                }else if(IGEL_TOKENS.count(buf)){
                    m_tokens.push_back({.type = IGEL_TOKENS.at(buf),.line = lineCount,.file = file});
                    buf.clear();
                    continue;
                }else if(REPLACE.count(buf)){
                    m_tokens.push_back(REPLACE.at(buf));
                    buf.clear();
                    continue;
                }else {
                    m_tokens.push_back({.type = TokenType::id,.value = buf,.line = lineCount,.file = file});
                    buf.clear();
                    continue;
                }
            }else if(std::isdigit(peak().value())){
                buf.push_back(consume());
                while (peak().has_value() && (std::isdigit(peak().value()) || peak().value() == '.') )
                {
                    buf.push_back(consume());
                }
                if(peak().has_value() && std::isalpha(peak().value()))buf.push_back(consume());
                m_tokens.push_back({.type = TokenType::int_lit,.value = buf,.line = lineCount,.file = file});
                buf.clear();
                continue;
            }else if(IGEL_TOKEN_CHAR.count(peak().value())){
                m_tokens.push_back({.type = IGEL_TOKEN_CHAR.at(consume()),.line = lineCount,.file = file});
                continue;
            }else if(std::isspace(peak().value())){
                consume();
                continue;
            }else {
                err("Unkown Symbol " + consume());
            }
        }

        if(comment == 2){
            err("Unclosed comment");
        }
        m_I = 0;
        return m_tokens;
    }