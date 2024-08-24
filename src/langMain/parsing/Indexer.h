//
// Created by igel on 18.03.24.
//

#ifndef IGEL_COMPILER_INDEXER_H
#define IGEL_COMPILER_INDEXER_H
#include <utility>

#include "../../CompilerInfo/InfoParser.h"
#include "../../types.h"

class Indexer {

public:
    Indexer(std::unordered_map<std::string,SrcFile*> file_table, std::unordered_map<std::string,Header*> header_table) : file_table(std::move(file_table)),header_table(std::move(header_table)) {
        init = true;
    }

    Indexer() = default;

    void setup(std::unordered_map<std::string,SrcFile*> file_table, std::unordered_map<std::string,Header*> header_table) {
        this->file_table = std::move(file_table);
        this->header_table = std::move(header_table);
        this->init = true;
    }

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
    std::unordered_map<std::string,SrcFile*> file_table;
    std::unordered_map<std::string,Header*> header_table;
    SrcFile* currentFile = nullptr;

    std::vector<Token> m_tokens;
    size_t m_I = 0;
    std::vector<BeContained*> m_super;

    bool init = false;

    void checkInit() {
        if(!init) {
            err("File has to be initalized before usage");
        }
    }
};


#endif //IGEL_COMPILER_INDEXER_H
