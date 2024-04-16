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
            m_super.back()->varTypes[tryConsume(TokenType::id,"Expected id").value.value()] = Types::VarType::PirimVar;
            consume();
        }else if(peak(1).has_value() && peak(2).has_value() && peak().value().type == TokenType::id && peak(1).value().type == TokenType::id &&
                peak(2).value().type != TokenType::openParenth) {
            std::string name = tryConsume(TokenType::id,"Expected id").value.value();
            std::string varName = tryConsume(TokenType::id,"Expected id").value.value();
            if(currentFile->findClass(name)) {
                m_super.back()->varTypes[varName] = Types::VarType::ClassVar;
                m_super.back()->varTypeNames[varName] = name;
            }else if(currentFile->findStruct(name)) {
                m_super.back()->varTypes[varName] = Types::VarType::StructVar;
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
            m_super.back()->varTypes[name] = Types::VarType::ArrayVar;
        }else if(peak().value().type >= TokenType::_struct && peak().value().type <= TokenType::_enum) {
            Token typeT = consume();
            IgType* type = currentFile->nameTypeMap[tryConsume(TokenType::id,"Expected id").value.value()];
            switch (typeT.type) {
                case TokenType::_struct:
                    m_super.push_back(type);
                    parseScope(false);
                    m_super.pop_back();
                    break;
                case TokenType::_class:
                case TokenType::_namespace:
                    m_super.push_back(type);
                    parseScope(true);
                    m_super.pop_back();
                    break;
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
