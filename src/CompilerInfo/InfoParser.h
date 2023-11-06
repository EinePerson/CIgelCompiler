//
// Created by igel on 06.11.23.
//

#ifndef IGEL_COMPILER_INFOPARSER_H
#define IGEL_COMPILER_INFOPARSER_H

#include <optional>
#include <string>
#include <vector>
#include <functional>
#include <sys/stat.h>
#include "../parsed.h"
#include "../util/arena.hpp"

class InfoParser;

struct Func{
    std::vector<std::string> args {};
    std::function<void(InfoParser*, std::vector<std::string>)> func;
};

enum class FunctionState{
    _include,
    _use,
    _inside,
};

struct FuncRet{
    Function* func;
    FunctionState state;
};

struct SrcFile{
    bool isMain = false;
    bool isGen = false;
    std::vector<SrcFile*> includes;
    std::vector<SrcFile*> _using;
    std::string name;
    //This is with the path
    std::string fullName;
    std::string fullNameOExt;
    std::vector<Token> tokens;
    size_t tokenPtr;
    std::vector<NodeStmt*> stmts;
    std::unordered_map<std::string,Function*> funcs;

    std::optional<FuncRet> findFunc(std::string name);
};

struct Directory{
    std::vector<SrcFile*> files;
    std::vector<Directory*> sub_dirs;
    std::string name;

    void genFile(std::string path);
};

struct Info{
    std::unordered_map<std::string,SrcFile*> file_table;
    std::vector<SrcFile*> files;
    Directory* src;
    std::vector<Func> calls;
    SrcFile* main;
    std::string* m_name;
};

struct FileItterator{
    std::vector<SrcFile*> files;
    Directory* dir{};
};

std::string removeExtension(const std::string& filename);
class InfoParser {
public:
    InfoParser(std::vector<Token> tokens,ArenaAlocator* alloc);

    static std::map<std::string,std::function<void(InfoParser*,std::vector<std::string>)>> funcs;

    Info* parse();

    std::optional<Func> parseFuncCall();

    FileItterator listFiles(std::string path,const std::string& name);

private:
    inline std::optional<Token> peak(int count = 0) const{
        if(m_I + count >= m_tokens.size())return {};
        else return m_tokens.at(m_I + count);
    }

    inline Token consume(){
        return m_tokens.at(m_I++);
    }

    inline Token tryConsume(TokenType type,const std::string& err){
        if(peak().has_value() && peak().value().type == type)return consume();
        else {
            std::cerr << err << "\n" << (peak().has_value() ? peak().value().file:peak(-1).value().file) << ": " << (peak().has_value() ? peak().value().line:peak(-1).value().line) << "\n";
            std::cerr << std::endl;
            exit(EXIT_FAILURE);
        }
    }

    inline std::optional<Token> tryConsume(TokenType type){
        if(peak().has_value() && peak().value().type == type)return consume();
        else return {};
    }

    std::vector<Token> m_tokens;
    size_t m_I = 0;
    ArenaAlocator* m_alloc;
    std::string m_main;
    Info* m_info;
};


#endif //IGEL_COMPILER_INFOPARSER_H
