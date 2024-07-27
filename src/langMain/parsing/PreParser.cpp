//
// Created by igel on 18.03.24.
//

#include "PreParser.h"

PreParser::PreParser(Info* info): m_info(info) {

}


void PreParser::parse() {
    for (auto file : m_info->file_table) {
        parse(file.second);
    }
}

void PreParser::parse(SrcFile* file) {
    currentFile = file;
    m_tokens = file->tokens;
    m_I = 0;

    parseScope(true,false);
}

void PreParser::parseScope(bool contain,bool parenth) {
    if(parenth) {
        //consume();
        //tryConsume(TokenType::id,"Expected id");
        tryConsume(TokenType::openCurl,"Expected '{'");
    }

    uint scopes = 0;
    while (peak().has_value()) {
        if(tryConsume(TokenType::openCurl)) {
            scopes++;
            continue;
        }else if(tryConsume(TokenType::closeCurl)) {
            if(scopes == 0) {
                m_I--;
                break;
            }
            scopes--;
            continue;
        }

        if(scopes != 0) {
            consume();
            continue;
        }

        if(peak(1).has_value() && peak(2).has_value() && (peak().value().type >= TokenType::_byte && peak().value().type <= TokenType::_bool)&& peak(1).value().type == TokenType::id &&
            peak(2).value().type != TokenType::openParenth) {
            consume();
            m_super.back()->varTypes[tryConsume(TokenType::id,"Expected id").value.value()] = Igel::VarType::PirimVar;
            consume();
        }else if(peak(1).has_value() && peak(2).has_value() && peak().value().type == TokenType::id && peak(1).value().type == TokenType::id &&
                peak(2).value().type != TokenType::openParenth) {
            std::string name = tryConsume(TokenType::id,"Expected id").value.value();
            std::string varName = tryConsume(TokenType::id,"Expected id").value.value();
            if(currentFile->findClass(name,nullptr)) {
                m_super.back()->varTypes[varName] = Igel::VarType::ClassVar;
                m_super.back()->varTypeNames[varName] = name;
            }else if(currentFile->findStruct(name,nullptr)) {
                m_super.back()->varTypes[varName] = Igel::VarType::StructVar;
                m_super.back()->varTypeNames[varName] = name;
            }
        }else if((peak().value().type == TokenType::id  || (peak().value().type >= TokenType::_byte && peak().value().type <= TokenType::_bool)) && peak(1).value().type == TokenType::openBracket) {
            consume();
            while (peak().value().type == TokenType::openBracket) {
                tryConsume(TokenType::openBracket,"Expected '['");
                tryConsume(TokenType::closeBracket,"Expected ']'");
            }
            std::string name = tryConsume(TokenType::id,"Expected id").value.value();
            if(peak().value().type == TokenType::openParenth)continue;
            m_super.back()->varTypes[name] = Igel::VarType::ArrayVar;
        }else if(peak().value().type >= TokenType::_struct && peak().value().type <= TokenType::_enum) {
            Token typeT = consume();
            std::string name = tryConsume(TokenType::id,"Expected id").value.value();
            IgType* type = currentFile->nameTypeMap[name];
            currentFile->unmangledTypeMap[name] = type;
            if(dynamic_cast<ContainableType*>(type) && !m_super.empty()) {
                currentFile->nameTypeMap.erase(name);
                dynamic_cast<ContainableType*>(type)->contType = m_super.back();
                currentFile->nameTypeMap[type->mangle()] = type;
            }
            switch (typeT.type) {
                case TokenType::_struct:
                    m_super.push_back(type);
                    parseScope(false);
                    m_super.pop_back();
                    break;
                case TokenType::_abstract:
                    if(peak().value().type != TokenType::_class)break;
                    consume();
                case TokenType::_class:
                    if(tryConsume(TokenType::extends)) {
                        if(parseContained())err("Expected type name");
                    }
                    if(tryConsume(TokenType::implements)) {
                        bool b = false;
                        while (tryConsume(TokenType::id)) {
                            b = tryConsume(TokenType::comma).has_value();
                        }
                        if(b)err("Expected type name");
                    }
                case TokenType::_namespace:
                    m_super.push_back(type);
                    parseScope(true);
                    m_super.pop_back();
                    break;
                case TokenType::interface:
                    if(tryConsume(TokenType::extends)) {
                        bool b = false;
                        while (tryConsume(TokenType::id)) {
                            b = tryConsume(TokenType::comma).has_value();
                        }
                        if(b)err("Expected type name");
                    }
                    m_super.push_back(type);
                    parseScope(true);
                    m_super.pop_back();
                    break;
                case TokenType::_enum: {
                    m_super.push_back(type);
                    Enum* _enum = dynamic_cast<Enum*>(type);
                    tryConsume(TokenType::openCurl,"Expected '{' after enum declaration");
                    int i = 0;
                    bool comma = false;
                    while (peak().has_value() && peak().value().type != TokenType::closeCurl) {
                        if(comma)err("Expected ','");
                        auto id = tryConsume(TokenType::id,"Expected id");
                        _enum->values.push_back(id.value.value());
                        _enum->valueIdMap[id.value.value()] = i;
                        i++;
                        comma = !tryConsume(TokenType::comma).has_value();
                    }
                    tryConsume(TokenType::comma);
                    tryConsume(TokenType::closeCurl);
                    m_super.pop_back();
                    break;
                }
                default: err(&"Unkown type: " [ static_cast<uint>(peak().value().type)]);
            }
        }else {
            consume();
        }
    }

    if(parenth) {
        tryConsume(TokenType::closeCurl,"Expected '}'");
    }
}

inline std::optional<Token> PreParser::peak(int count) const{
    if(m_I + count >= m_tokens.size())return {};
    else return m_tokens.at(m_I + count);
}

inline Token PreParser::consume(){
    return m_tokens.at(m_I++);
}

inline Token PreParser::tryConsume(TokenType type,const std::string& errs){
    if(peak().has_value() && peak().value().type == type)return consume();
    else {
        err(errs);
    }
}

inline std::optional<Token> PreParser::tryConsume(TokenType type){
    if(peak().has_value() && peak().value().type == type)return consume();
    else return {};
}

void PreParser::err(const std::string&err) const {
    std::cerr << err << "\n" << "   at: " << (peak().has_value()?peak().value().file:peak(-1).value().file) << ":" << (peak().has_value()?peak().value().line:peak(-1).value().line) << ":"
    << (peak().has_value()?peak().value()._char:peak(-1).value()._char) << std::endl;
    exit(EXIT_FAILURE);
}

bool PreParser::parseContained() {
    while (peak().value().type == TokenType::id && peak(1).value().type == TokenType::dConnect) {
        tryConsume(TokenType::id);
        tryConsume(TokenType::dConnect);
    }
    return !tryConsume(TokenType::id).has_value();
}
