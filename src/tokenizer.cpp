#include "tokenizer.h"

#include "langMain/codeGen/Generator.h"
#include "util/Mangler.h"

typedef unsigned int uint;

    std::map<std::string,std::function<llvm::FunctionCallee()>> Tokenizer::LIB_FUNCS {
        {"_Z4exit",[]() -> llvm::FunctionCallee {
            std::vector<Type*> params = {Type::getInt32Ty(*Generator::instance->m_contxt)};
            FunctionType *fType = FunctionType::get(Type::getVoidTy(*Generator::instance->m_contxt), params, false);
            return Generator::instance->m_module->getOrInsertFunction("exit",fType);
        }},
        {"_Z6printf",[]() -> llvm::FunctionCallee {
            std::vector<Type*> params = {PointerType::get(*Generator::instance->m_contxt,0)};
            FunctionType *fType = FunctionType::get(Type::getVoidTy(*Generator::instance->m_contxt), params, true);
            return Generator::instance->m_module->getOrInsertFunction("printf",fType);
        }},
    };

    std::optional<int> prec(TokenType type){
        switch (type)
        {
            case TokenType::question:
                return 0;
            case TokenType::bigequal:
            case TokenType::equal:
            case TokenType::notequal:
            case TokenType::smallequal:
            case TokenType::small:
            case TokenType::big:
            case TokenType::_bitAnd:
            case TokenType::_bitOr:
                return 1;
            case TokenType::plus:
            case TokenType::sub:
                return 2;
            case TokenType::div:
            case TokenType::mul:
            case TokenType::mod:
                return 3;
            case TokenType::pow:
                return 4;
            default:
                return {};
        }
    }

    Tokenizer:: Tokenizer(const std::set<std::string>& extended_funcs) : m_extended_funcs(extended_funcs){
        if(merged)return;
        merged = true;
        for (const auto &item: FLAGS){
            REPLACE[item.first] = Token(TokenType::int_lit,(uint) -1,-1,"",std::to_string(item.second));
        }
    }

     std::vector<Token> Tokenizer::tokenize(const std::string&file){
        m_src = read(file);
        m_I = 0;
        m_tokens = {};
        std::string buf;

        char comment = 0;
        lineCount = 1;
        charCount = 0;
        while (peak().has_value())
        {
            if(peak().value() == '\n') {
                //lineCount++;
                charCount = 0;
            }
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
                    m_tokens.emplace_back(TokenType::div_eq,lineCount,charCount,file);
                    consume();
                    consume();
                    continue;
                }else {
                    m_tokens.emplace_back(TokenType::div,lineCount,charCount,file);
                    consume();
                    continue;
                }
            }else if(peak().value() == '?') {
                consume();
                m_tokens.emplace_back(TokenType::question,lineCount,charCount,file);
                continue;
            }else if(peak().value() == '!'){
                consume();
                if(peak().has_value() && peak().value() == '='){
                    m_tokens.emplace_back(TokenType::notequal,lineCount,charCount,file);
                    consume();
                    continue;
                }else if(peak().has_value() && peak().value() == '|'){
                    m_tokens.emplace_back(TokenType::_xor,lineCount,charCount,file);
                    consume();
                    continue;
                }else{
                    m_tokens.emplace_back(TokenType::_not,lineCount,charCount,file);
                }
            }else if(peak().value() == '>'){
                consume();
                if(peak().has_value() && peak().value() == '='){
                    m_tokens.emplace_back(TokenType::bigequal,lineCount,charCount,file);
                    consume();
                    continue;
                }else{
                    m_tokens.emplace_back(TokenType::big,lineCount,charCount,file);
                    continue;
                }
            }else if(peak().value() == '<'){
                consume();
                if(peak().has_value() && peak().value() == '='){
                    m_tokens.emplace_back(TokenType::smallequal,lineCount,charCount,file);
                    consume();
                    continue;
                }else{
                    m_tokens.emplace_back(TokenType::small,lineCount,charCount,file);
                    continue;
                }
            }else if(peak().value() == '='){
                consume();
                if(peak().has_value() && peak().value() == '='){
                    m_tokens.emplace_back(TokenType::equal,lineCount,charCount,file);
                    consume();
                    continue;
                }else {
                    m_tokens.emplace_back(TokenType::eq,lineCount,charCount,file);
                    consume();
                    continue;
                }
            }else if(peak().value() == '&'){
                consume();
                if(peak().has_value() && peak().value() == '&'){
                    m_tokens.emplace_back(TokenType::_and,lineCount,charCount,file);
                    consume();
                    continue;
                }else{
                    m_tokens.emplace_back(TokenType::_bitAnd,lineCount,charCount,file);
                }
            }else if(peak().value() == '|'){
                consume();
                if(peak().has_value() && peak().value() == '|'){
                    m_tokens.emplace_back(TokenType::_or,lineCount,charCount,file);
                    consume();
                    continue;
                }else{
                    m_tokens.emplace_back(TokenType::_bitOr,lineCount,charCount,file);
                }
            }else if(peak().value() == '+'){
                consume();
                if(peak().has_value() && peak().value() == '='){
                    m_tokens.emplace_back(TokenType::plus_eq,lineCount,charCount,file);
                    consume();
                    continue;
                }else if(peak().has_value() && peak().value() == '+'){
                    m_tokens.emplace_back(TokenType::inc,lineCount,charCount,file);
                    consume();
                    continue;
                }else {
                    m_tokens.emplace_back(TokenType::plus,lineCount,charCount,file);
                    consume();
                    continue;
                }
            }else if(peak().value() == '-'){
                consume();
                if(peak().has_value() && peak().value() == '='){
                    m_tokens.emplace_back(TokenType::sub_eq,lineCount,charCount,file);
                    consume();
                    continue;
                }else if(peak().has_value() && peak().value() == '-'){
                    m_tokens.emplace_back(TokenType::dec,lineCount,charCount,file);
                    consume();
                    continue;
                }else if(peak().has_value() && peak().value() == '>') {
                    m_tokens.emplace_back(TokenType::connector,lineCount,charCount,file);
                    consume();
                    continue;
                }else if(std::isdigit(peak().value())){
                    buf.push_back('-');
                    buf.push_back(consume());
                    while (isNum())
                    {
                        buf.push_back(consume());
                    }
                    if(peak().has_value() && std::isalpha(peak().value()))buf.push_back(consume());
                    m_tokens.emplace_back(TokenType::int_lit,lineCount,charCount,file,buf);
                    buf.clear();
                    continue;
                }else{
                    m_tokens.emplace_back(TokenType::sub,lineCount,charCount,file);
                    consume();
                    continue;
                }
            }else if(peak().value() == '*'){
                consume();
                if(peak().has_value() && peak().value() == '='){
                    m_tokens.emplace_back(TokenType::mul_eq,lineCount,charCount,file);
                    consume();
                    continue;
                }else {
                    m_tokens.emplace_back(TokenType::mul,lineCount,charCount,file);
                    consume();
                    continue;
                }
            }else if(peak().value() == '/'){
                consume();
                if(peak().has_value() && peak().value() == '='){
                    m_tokens.emplace_back(TokenType::div_eq,lineCount,charCount,file);
                    consume();
                    continue;
                }else {
                    m_tokens.emplace_back(TokenType::div,lineCount,charCount,file);
                    consume();
                    continue;
                }
            }else if(peak().value() == '*'){
                consume();
                if(peak().has_value() && peak().value() == '='){
                    m_tokens.emplace_back(TokenType::pow_eq,lineCount,charCount,file);
                    consume();
                    continue;
                }else {
                    m_tokens.emplace_back(TokenType::pow,lineCount,charCount,file);
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
                m_tokens.emplace_back(TokenType::str,lineCount,charCount,file,buf);
                buf.clear();
                continue;
            }else if(peak().value() == '\'') {
                consume();
                if(peak().has_value() && peak().value() != '\'')m_tokens.emplace_back(TokenType::_char_lit,lineCount,charCount,file,std::string{consume()});
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
                std::transform(buf.begin(), buf.end(),buf.begin(),[](unsigned char c){ return std::tolower(c); });
                m_tokens.emplace_back(TokenType::info,lineCount,charCount,file,buf);
                buf.clear();
                continue;
            }else if(peak().value() == ':') {
                consume();
                if(peak().value() == ':') {
                    consume();
                    m_tokens.emplace_back(TokenType::dConnect,lineCount,charCount,file);
                    continue;
                }
                m_tokens.emplace_back(TokenType::next,lineCount,charCount,file);
                continue;
            }else if(std::isalpha(peak().value()) || peak().value() == '_'){
                buf.push_back(consume());
                while (peak().has_value() && (std::isalnum(peak().value()) || peak().value() == '_')){
                    buf.push_back(consume());
                }
                while(peak().has_value() && std::isspace(peak().value())){
                    consume();
                }

                if(IGEL_TOKENS.count(buf)){
                    m_tokens.emplace_back(IGEL_TOKENS.at(buf),lineCount,charCount,file);
                    buf.clear();
                    continue;
                }else if(REPLACE.count(buf)){
                    Token t = REPLACE.at(buf);
                    t.line = lineCount;
                    t.file = file;
                    m_tokens.push_back(t);
                    buf.clear();
                    continue;
                }else if(m_extended_funcs.count(buf)) {
                    m_tokens.emplace_back(TokenType::externFunc,lineCount,charCount,file,buf);
                    buf.clear();
                }else{
                    m_tokens.emplace_back(TokenType::id,lineCount,charCount,file,buf);
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
                m_tokens.emplace_back(TokenType::int_lit,lineCount,charCount,file,buf);
                buf.clear();
                continue;
            }else if(IGEL_TOKEN_CHAR.count(peak().value())){
                m_tokens.emplace_back(IGEL_TOKEN_CHAR.at(consume()),lineCount,charCount,file);
                continue;
            }else if(std::isspace(peak().value())){
                consume();
                continue;
            }else {
                std::string er = "Unkown Symbol ";
                er += consume();
                err(er);
            }
        }

        if(comment == 2){
            err("Unclosed comment");
        }
        m_I = 0;
        return m_tokens;
    }