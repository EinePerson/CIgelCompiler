#include "tokenizer.h"

#include "langMain/codeGen/Generator.h"

typedef unsigned int uint;

    std::map<std::string,std::function<llvm::FunctionCallee()>> Tokenizer::LIB_FUNCS {
        {"_Z4exit",[]() -> FunctionCallee {
            std::vector<Type*> params = {Type::getInt32Ty(*Generator::instance->m_contxt)};
            FunctionType *fType = FunctionType::get(Type::getVoidTy(*Generator::instance->m_contxt), params, false);
            return Generator::instance->m_module->getOrInsertFunction("exit",fType);
        }},
        {"_Z6printf",[]() -> FunctionCallee {
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
            case TokenType::_bitAnd:
            case TokenType::_bitOr:
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
                    m_tokens.emplace_back(TokenType::div_eq,lineCount,file);
                    consume();
                    consume();
                    continue;
                }else {
                    m_tokens.emplace_back(TokenType::div,lineCount,file);
                    consume();
                    continue;
                }
            }else if(peak().value() == '!'){
                consume();
                if(peak().has_value() && peak().value() == '='){
                    m_tokens.emplace_back(TokenType::notequal,lineCount,file);
                    consume();
                    continue;
                }else if(peak().has_value() && peak().value() == '|'){
                    m_tokens.emplace_back(TokenType::_xor,lineCount,file);
                    consume();
                    continue;
                }else{
                    m_tokens.emplace_back(TokenType::_not,lineCount,file);
                }
            }else if(peak().value() == '>'){
                consume();
                if(peak().has_value() && peak().value() == '='){
                    m_tokens.emplace_back(TokenType::bigequal,lineCount,file);
                    consume();
                    continue;
                }else{
                    m_tokens.emplace_back(TokenType::big,lineCount,file);
                    continue;
                }
            }else if(peak().value() == '<'){
                consume();
                if(peak().has_value() && peak().value() == '='){
                    m_tokens.emplace_back(TokenType::smallequal,lineCount,file);
                    consume();
                    continue;
                }else{
                    m_tokens.emplace_back(TokenType::small,lineCount,file);
                    continue;
                }
            }else if(peak().value() == '='){
                consume();
                if(peak().has_value() && peak().value() == '='){
                    m_tokens.emplace_back(TokenType::equal,lineCount,file);
                    consume();
                    continue;
                }else {
                    m_tokens.emplace_back(TokenType::eq,lineCount,file);
                    consume();
                    continue;
                }
            }else if(peak().value() == '&'){
                consume();
                if(peak().has_value() && peak().value() == '&'){
                    m_tokens.emplace_back(TokenType::_and,lineCount,file);
                    consume();
                    continue;
                }else{
                    m_tokens.emplace_back(TokenType::_bitAnd,lineCount,file);
                }
            }else if(peak().value() == '|'){
                consume();
                if(peak().has_value() && peak().value() == '|'){
                    m_tokens.emplace_back(TokenType::_or,lineCount,file);
                    consume();
                    continue;
                }else{
                    m_tokens.emplace_back(TokenType::_bitOr,lineCount,file);
                }
            }else if(peak().value() == '+'){
                consume();
                if(peak().has_value() && peak().value() == '='){
                    m_tokens.emplace_back(TokenType::plus_eq,lineCount,file);
                    consume();
                    continue;
                }else if(peak().has_value() && peak().value() == '+'){
                    m_tokens.emplace_back(TokenType::inc,lineCount,file);
                    consume();
                    continue;
                }else {
                    m_tokens.emplace_back(TokenType::plus,lineCount,file);
                    consume();
                    continue;
                }
            }else if(peak().value() == '-'){
                consume();
                if(peak().has_value() && peak().value() == '='){
                    m_tokens.emplace_back(TokenType::sub_eq,lineCount,file);
                    consume();
                    continue;
                }else if(peak().has_value() && peak().value() == '-'){
                    m_tokens.emplace_back(TokenType::dec,lineCount,file);
                    consume();
                    continue;
                }else {
                    m_tokens.emplace_back(TokenType::sub,lineCount,file);
                    consume();
                    continue;
                }
            }else if(peak().value() == '*'){
                consume();
                if(peak().has_value() && peak().value() == '='){
                    m_tokens.emplace_back(TokenType::mul_eq,lineCount,file);
                    consume();
                    continue;
                }else {
                    m_tokens.emplace_back(TokenType::mul,lineCount,file);
                    consume();
                    continue;
                }
            }else if(peak().value() == '/'){
                consume();
                if(peak().has_value() && peak().value() == '='){
                    m_tokens.emplace_back(TokenType::div_eq,lineCount,file);
                    consume();
                    continue;
                }else {
                    m_tokens.emplace_back(TokenType::div,lineCount,file);
                    consume();
                    continue;
                }
            }else if(peak().value() == '*'){
                consume();
                if(peak().has_value() && peak().value() == '='){
                    m_tokens.emplace_back(TokenType::pow_eq,lineCount,file);
                    consume();
                    continue;
                }else {
                    m_tokens.emplace_back(TokenType::pow,lineCount,file);
                    consume();
                    continue;
                }
            }else if(peak().value() == '"'){
                consume();
                while (peak().has_value() && peak().value() != '"')
                {
                    if(peak().value() == '\\') {
                        consume();
                        if(ESCAPES.contains(peak().value())) {
                            buf.append(ESCAPES.at(consume()));
                            continue;
                        }
                    }
                    buf.push_back(consume());
                }
                consume();
                m_tokens.emplace_back(TokenType::str,lineCount,file,buf);
                buf.clear();
                continue;
            }else if(peak().value() == '\'') {
                consume();
                if(peak().has_value() && peak().value() != '\'')m_tokens.emplace_back(TokenType::_char_lit,lineCount,file,std::string{consume()});
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
                m_tokens.emplace_back(TokenType::info,lineCount,file,buf);
                buf.clear();
                continue;
            }else if(peak().value() == ':') {
                consume();
                if(peak().value() == ':') {
                    consume();
                    m_tokens.emplace_back(TokenType::dConnect,lineCount,file);
                    continue;
                }
                m_tokens.emplace_back(TokenType::next,lineCount,file);
                continue;
            }else if(std::isalpha(peak().value()) || peak().value() == '_'){
                buf.push_back(consume());
                while (peak().has_value() && (std::isalnum(peak().value()) || peak().value() == '_'))
                {
                    buf.push_back(consume());
                }
                while(peak().has_value() && std::isspace(peak().value())){
                    consume();
                }
                if(peak().has_value() && peak().value() == '('){
                    if(FUNCTIONS.count(buf)){
                        m_tokens.emplace_back(FUNCTIONS.at(buf),lineCount,file);
                        buf.clear();
                        continue;
                    }else if(m_extended_funcs.count(buf)){
                        m_tokens.emplace_back(TokenType::externFunc,lineCount,file,buf);
                        buf.clear();
                        continue;
                    }else{
                        m_tokens.emplace_back(TokenType::id,lineCount,file,buf);
                        buf.clear();
                        continue;
                    }
                }else if(IGEL_TOKENS.count(buf)){
                    m_tokens.emplace_back(IGEL_TOKENS.at(buf),lineCount,file);
                    buf.clear();
                    continue;
                }else if(REPLACE.count(buf)){
                    Token t = REPLACE.at(buf);
                    t.line = lineCount;
                    t.file = file;
                    m_tokens.push_back(t);
                    buf.clear();
                    continue;
                }else {
                    m_tokens.emplace_back(TokenType::id,lineCount,file,buf);
                    buf.clear();
                    continue;
                }
            }else if(std::isdigit(peak().value())){
                buf.push_back(consume());
                while (isNum())
                {
                    buf.push_back(consume());
                }
                if(peak().has_value() && std::isalpha(peak().value()))buf.push_back(consume());
                m_tokens.emplace_back(TokenType::int_lit,lineCount,file,buf);
                buf.clear();
                continue;
            }else if(IGEL_TOKEN_CHAR.count(peak().value())){
                m_tokens.emplace_back(IGEL_TOKEN_CHAR.at(consume()),lineCount,file);
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