//
// Created by igel on 18.03.24.
//

#ifndef IGEL_COMPILER_PREPARSER_H
#define IGEL_COMPILER_PREPARSER_H
#include "../../CompilerInfo/InfoParser.h"


class PreParser {

public:
    explicit PreParser(Info* info);

    void parse();

    void parse(SrcFile* file);

    void parseScope(bool contain,bool parenth = true);

    std::optional<Token> peak(int count = 0) const;

    Token consume();

    Token tryConsume(TokenType type, const std::string&errs);

    std::optional<Token> tryConsume(TokenType type);

    void err(const std::string&err) const;

    ///\brief checks weather the value is a type <br> returns true for no type
    bool parseContained();

private:
    Info* m_info;
    SrcFile* currentFile = nullptr;
    std::vector<Token> m_tokens;
    size_t m_I = 0;
    std::vector<IgType*> m_super;
};


#endif //IGEL_COMPILER_PREPARSER_H
