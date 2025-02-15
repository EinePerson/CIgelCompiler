//
// Created by igel on 12.01.24.
//

#ifndef PARSER_H
#define PARSER_H

#include <vector>
#include <iostream>

#include "../codeGen/Generator.h"

#define TEMPLATE_EMPTY_NAME "templateEmptyPlaceholderInterface"

class Parser {
public:
    Parser(){
        templateEmpty->name = TEMPLATE_EMPTY_NAME;
        templateEmpty->overrideSub = true;
        ptr_type->name = "Pointer";
    }

    static char sidChar(char c) {
        c = std::toupper(c);
        switch (c) {
            case 'Z': return 10;
            case 'D': return 9;
            case 'F': return 8;
            case 'L': return 3;
            case 'I': return 2;
            case 'S': return 1;
            case 'B': return 0;
            default: return 2;
        }
    }

    std::optional<NodeTerm*> parseTerm(std::optional<BeContained*> cont = {},NodeTerm* term = nullptr);
    std::optional<NodeExpr*> parseExpr(int minPrec = 0);
    std::optional<NodeStmt*> parseStmt(bool semi = true);
    std::optional<NodeStmt*> parseTermToStmt(NodeTerm* term,bool semi = true);

    std::optional<NodeStmtLet*> parseLet(bool _static,bool final,std::optional<BeContained*> contP = {});
    std::optional<NodeStmtPirimitiv*> parsePirim(bool _static,bool final);
    std::optional<NodeStmtNew*> parseNew(bool _static,bool final,std::optional<BeContained*> contP = {});
    std::optional<NodeStmtArr*> parseArrNew(bool _static,bool final,std::optional<BeContained*> contP = {});
    std::optional<NodeStmtEnum*> parseEnum(bool _static,bool final,std::optional<BeContained*> contP = {});

    std::optional<NodeStmtScope*> parseScope();
    NodeStmtFor* parseFor();
    NodeStmtWhile* parseWhile();
    NodeStmtIf* parseIf();
    std::optional<IgFunction*> parseFunc(bool constructor = false);
    std::optional<std::pair<BeContained*,int>> parseContained();
    std::optional<BeContained*> parseContained(std::string str);
    std::optional<std::pair<IgType*,int>> findContained(std::string name);

    std::optional<IgType*> parseType();

    SrcFile* parseProg(SrcFile* file);

    bool isprocedingFund();

    Igel::Access parseAccess();

    std::optional<GeneratedType*> parseComplexType(std::optional<BeContained*> cont = {});

private:
    Position currentPosition() {
        return {peak().value().line,peak().value()._char,peak().value().file,};
    }

    [[nodiscard]] std::optional<Token> peak(const int count = 0) const{
        if(m_I + count >= m_tokens.size())return {};
        return m_tokens.at(m_I + count);
    }

    Token consume(){
        return m_tokens.at(m_I++);
    }

    Token tryConsume(const TokenType type,const std::string& errs){
        if(peak().has_value() && peak().value().type == type)return consume();
        err(errs);
        return {};
    }

    std::optional<Token> tryConsume(const TokenType type){
        if(peak().has_value() && peak().value().type == type)return consume();
        else return {};
    }

    void err(const std::string&err) const {
        std::cerr << err << "\n" << "   at: " << (peak().has_value()?peak().value().file:peak(-1).value().file) << ":" << (peak().has_value()?peak().value().line:peak(-1).value().line) << ":"
        << (peak().has_value()?peak().value()._char:peak(-1).value()._char) << std::endl;
        exit(EXIT_FAILURE);
    }

    std::vector<Token> m_tokens;
    size_t m_I = 0;
    char m_sidFlag = -1;
    SrcFile* m_file = nullptr;
    std::vector<ContainableType*> m_super {};
    IgType* varHolder = nullptr;
    std::vector<std::unordered_map<std::string,Igel::VarType>> varTypes;
    std::vector<std::unordered_map<std::string,IgType*>> templateTypes;

public:
    static Interface* templateEmpty;
    static Interface* ptr_type;
};



#endif //PARSER_H
