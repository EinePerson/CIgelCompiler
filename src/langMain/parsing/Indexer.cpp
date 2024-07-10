//
// Created by igel on 18.03.24.
//

#include "Indexer.h"

Indexer::Indexer(Info* info):m_info(info) {
}

void Indexer::index() {
    for (auto file : m_info->file_table) {
        index(file.second);
    }
}

void Indexer::index(SrcFile* file) {
    currentFile = file;

    m_tokens = file->tokens;
    m_I = 0;
    while(peak().has_value()){
        char b = 0;
        b = tryParseInclude();
        b += parseUsing();
        if(!b)break;
    }
    while (peak().has_value()) {
        if((peak().value().type >= TokenType::_struct && peak().value().type <= TokenType::_enum) || peak().value().type == TokenType::_abstract) {
            IgType* type = nullptr;
            bool cont = false;
            switch (peak().value().type) {
                case TokenType::_struct:  type = new Struct;
                    cont = false;
                    if(!m_super.empty())type->contType = m_super.back();
                    break;
                case TokenType::_abstract:
                    consume();
                    if(peak().value().type != TokenType::_class) {
                        consume();
                        continue;
                    }
                    type = new Class;
                    dynamic_cast<Class*>(type)->abstract = true;
                    if(!m_super.empty())type->contType = m_super.back();
                    m_super.push_back(type);
                    cont = true;
                    break;
                case TokenType::_class:
                    type = new Class;
                    if(!m_super.empty())type->contType = m_super.back();
                    m_super.push_back(type);
                    cont = true;
                    break;
                case TokenType::_namespace: type = new NamesSpace;
                    if(!m_super.empty())type->contType = m_super.back();
                    m_super.push_back(type);
                    cont = true;
                    break;
                case TokenType::interface: type = new Interface;
                    if(!m_super.empty())type->contType = m_super.back();
                    m_super.push_back(type);
                    cont = true;
                    break;
                case TokenType::_enum: type = new Enum;
                    if(!m_super.empty())type->contType = m_super.back();
                    m_super.push_back(type);
                    cont = false;
                    break;
                default: err(&"Unkown type: " [ static_cast<uint>(peak().value().type)]);
            }
            consume();
            if(peak().value().type != TokenType::id)err("Expected type name");
            type->name = peak().value().value.value();
            file->nameTypeMap[type->mangle()] = type;
            if(cont)m_super.pop_back();
            file->typeNames.push_back(consume().value.value());
        }else consume();
    }
}

SrcFile* Indexer::getFile(std::string name) {
    if(m_info->file_table.contains(name))return m_info->file_table[name];
    err("Unknown file " + name);
    return nullptr;
}

bool Indexer::tryParseInclude(){
    //TODO ADD HEADER INCLUDING
    if(peak().has_value() && peak().value().type == TokenType::include){
        consume();
        auto id = tryConsume(TokenType::str,"Expected identifier").value;
        tryConsume(TokenType::semi,"Expected ';'");
        currentFile->tokenPtr += 3;
        return true;
    }else return false;
}

bool Indexer::parseUsing(){
    if(peak().has_value() && peak().value().type == TokenType::use){
        consume();
        auto id = tryConsume(TokenType::str,"Expected identifier").value.value();
        currentFile->_using.push_back(getFile(id));
        tryConsume(TokenType::semi,"Expected ';'");
        currentFile->tokenPtr += 3;
        return true;
    }else return false;
}

inline std::optional<Token> Indexer::peak(int count) const{
    if(m_I + count >= m_tokens.size())return {};
    else return m_tokens.at(m_I + count);
}

inline Token Indexer::consume(){
    return m_tokens.at(m_I++);
}

inline Token Indexer::tryConsume(TokenType type,const std::string& errs){
    if(peak().has_value() && peak().value().type == type)return consume();
    err(errs);
}

inline std::optional<Token> Indexer::tryConsume(TokenType type){
    if(peak().has_value() && peak().value().type == type)return consume();
    else return {};
}

void Indexer::err(const std::string&err) const {
    std::cerr << err << "\n" << "   at: " << (peak().has_value()?peak().value().file:peak(-1).value().file) << ":" << (peak().has_value()?peak().value().line:peak(-1).value().line) << ":"
    << (peak().has_value()?peak().value()._char:peak(-1).value()._char) << std::endl;
    exit(EXIT_FAILURE);
}
