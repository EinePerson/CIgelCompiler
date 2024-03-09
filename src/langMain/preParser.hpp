#pragma once
#include <algorithm>

#include "../CompilerInfo/CompilerInfo.hpp"
#include "parsing/Parser.h"

class PreParser{

public:
    PreParser(Info* info): m_info(info),m_parser() {
        for(auto file:info->files){
            m_unParsed.insert(std::pair(file,false));
        }
    }

    void parse(Info* info){
        m_info = info;
        for(auto pair:m_unParsed){
            if(!pair.second){
                parse(pair.first);
                m_unParsed[pair.first] = true;
            }
        }
        
    }

    void parse(SrcFile* file){
        m_file = file;
        m_tokens = m_file->tokens;
        m_I = 0;
        while(peak().has_value()){
            char b = 0;
            b = tryParseInclude();
            b += parseUsing();
            if(!b)break;
        }
        while (peak().has_value()) {
            if(peak().value().type >= TokenType::_struct && peak().value().type <= TokenType::_enum) {
                IgType* type = nullptr;
                switch (peak().value().type) {
                    case TokenType::_struct:  type = new Struct;
                        break;
                    case TokenType::_class: type = new Class;
                        break;
                    case TokenType::_namespace: type = new NamesSpace;
                        break;
                    default: err("Unkown type: " + static_cast<uint>(peak().value().type));
                }
                consume();
                if(peak().value().type != TokenType::id)err("Expected type name");
                file->nameTypeMap[peak().value().value.value()] = type;
                type->name = peak().value().value.value();
                file->typeNames.push_back(consume().value.value());
            }else consume();
        }
        m_parser.parseProg(file);
    }

    SrcFile* getFile(std::string name){
        for (auto src : m_info->src) {
            std::string str = src->name;
            str += name;
            if(m_info->file_table.count(str)){
                SrcFile* file = m_info->file_table.at(str);
                if(m_unParsed.contains(file)){
                    m_parser.parseProg(file);
                    m_unParsed.erase(file);
                }
                return file;
            }
        }

        std::cout << "Could not find file:" << name << "\nAt " << m_tokens.at(m_file->tokenPtr).file << ":" << m_tokens.at(m_file->tokenPtr).line << std::endl;
        exit(EXIT_FAILURE);
    }

    Header* getHeader(std::string name){
        for (auto src : m_info->src) {
            std::string str = src->name;
            str += name;
            if(m_info->file_table.count(str)){
                Header* file = m_info->header_table.at(str);
                return file;
            }
        }

        std::cout << "Could not find file:" << name << "\nAt " << m_tokens.at(m_file->tokenPtr).file << ":" << m_tokens.at(m_file->tokenPtr).line << std::endl;
        exit(EXIT_FAILURE);
    }

    bool tryParseInclude(){
        if(peak().has_value() && peak().value().type == TokenType::include){
            consume();
            auto id = tryConsume(TokenType::str,"Expected identifier").value;
            tryConsume(TokenType::semi,"Expected ';'");
            m_file->tokenPtr += 3;
            return true;
        }else return false;
    }

    bool parseUsing(){
        if(peak().has_value() && peak().value().type == TokenType::use){
            consume();
            auto id = tryConsume(TokenType::str,"Expected identifier").value.value();
            m_file->_using.push_back(getFile(id));
            tryConsume(TokenType::semi,"Expected ';'");
            m_file->tokenPtr += 3;
            return true;
        }else return false;
    }

    inline std::optional<Token> peak(int count = 0) const{
        if(m_I + count >= m_tokens.size())return {};
        else return m_tokens.at(m_I + count);
    }

    inline Token consume(){
        return m_tokens.at(m_I++);
    }

    inline Token tryConsume(TokenType type,const std::string& errs){
        if(peak().has_value() && peak().value().type == type)return consume();
        else {
            err(errs);
        }
    }

    inline std::optional<Token> tryConsume(TokenType type){
        if(peak().has_value() && peak().value().type == type)return consume();
        else return {};
    }

    void err(const std::string&err) const {
        std::cerr << err << "\n" << "   at: " << peak(-1).value().file << ":" << peak(-1).value().line << std::endl;
        exit(EXIT_FAILURE);
    }

    ~PreParser() = default;

    private:
        Info* m_info;
        SrcFile* m_file;
        Parser m_parser;
        std::vector<Token> m_tokens;
        std::unordered_map<SrcFile*,bool> m_unParsed {};
        size_t m_I = 0;
};