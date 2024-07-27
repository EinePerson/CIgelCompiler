//
// Created by igel on 18.03.24.
//

#ifndef IGEL_COMPILER_INDEXER_H
#define IGEL_COMPILER_INDEXER_H
#include "../../CompilerInfo/InfoParser.h"


class Indexer {

public:
    explicit Indexer(Info* info);

    void index();

    void index(SrcFile* file);

    SrcFile* getFile(std::string name);

    Header* getHeader(std::string header);

    bool tryParseInclude();

    bool parseUsing();

    std::optional<Token> peak(int count = 0) const;

    Token consume();

    Token tryConsume(TokenType type, const std::string&errs);

    std::optional<Token> tryConsume(TokenType type);

    void err(const std::string&err) const;

private:
    Info* m_info;
    SrcFile* currentFile = nullptr;

    std::vector<Token> m_tokens;
    size_t m_I = 0;
    std::vector<BeContained*> m_super;
};


#endif //IGEL_COMPILER_INDEXER_H
