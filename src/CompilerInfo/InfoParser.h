//
// Created by igel on 06.11.23.
//

#ifndef IGEL_COMPILER_INFOPARSER_H
#define IGEL_COMPILER_INFOPARSER_H

#include <optional>
#include <string>
#include <utility>
#include <vector>
#include <functional>
#include <sys/stat.h>
#include "../types.h"

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

//Compiler Info FLAGS
const std::map<std::string,uint> FLAGS = {
    {"Debug",0b1},
    {"Optimize",0b10},
};
//#define IS_DEBUG 0b1

struct FuncSig {
    FuncSig(std::string name, const std::vector<llvm::Type*>&types) : name(name),types(types),_return(nullptr){}
    FuncSig(std::string name, const std::vector<llvm::Type*>&types,llvm::Type* _return) : name(name),types(types),_return(_return){}
    std::string name;
    std::vector<llvm::Type*> types;
    llvm::Type* _return;

    bool operator==(const FuncSig& sig) const {
        if(name != sig.name)return false;
        if(types.size() != sig.types.size())return false;
        for(size_t i = 0;i < types.size();i++) {
            if(types[i] != sig.types[i])return false;
        }
        if(_return != nullptr && sig._return != nullptr)return _return == sig._return;
        return true;
    }
};

struct FuncSigHash {
    ulong operator()(const FuncSig& sig) const {
        ulong hash = std::hash<std::string>{}(sig.name);
        for (auto type : sig.types)hash += reinterpret_cast<ulong>(type);
        return hash;
    }
};

struct Header {
    std::string fullName;
    std::vector<FuncSig*> funcs;
    std::unordered_map<std::string,llvm::StructType*> structs;
    std::optional<std::pair<llvm::FunctionCallee,bool>> findFunc(std::string name,std::vector<llvm::Type*> types);
    std::optional<IgFunction*> findIgFunc(std::string name, std::vector<llvm::Type*> types);
    std::optional<std::pair<llvm::StructType*,Struct*>> findStruct(std::string name);
};

struct SrcFile{
    bool isMain = false;
    bool isGen = false;
    std::vector<Header*> includes;
    std::vector<SrcFile*> _using;
    std::string name;
    //This is with the path
    std::string fullName;
    std::string fullNameOExt;
    std::vector<Token> tokens;
    size_t tokenPtr;
    std::vector<NodeStmt*> stmts;
    std::vector<IgType*> types;
    std::vector<std::string> typeNames;
    std::unordered_map<FuncSig,IgFunction*,FuncSigHash> funcs;
    std::unordered_map<std::string,std::pair<llvm::StructType*,Struct*>> structs;

    std::optional<std::pair<llvm::FunctionCallee,bool>> findFunc(std::string name, std::vector<llvm::Type*> types);
    std::optional<IgFunction*> findIgFunc(std::string name, std::vector<llvm::Type*> types);
    std::optional<std::pair<llvm::StructType*,Struct*>> findStruct(std::string name);
};

struct Directory{
    std::vector<SrcFile*> files;
    std::vector<Header*> headers;
    std::vector<Directory*> sub_dirs;
    std::vector<Directory*> includes;
    std::string name;

    void genFile(std::string path);
};

struct Info{
    std::vector<SrcFile*> files;
    std::vector<Header*> headers;
    std::vector<Directory*> src;
    std::vector<Directory*> include;
    std::vector<Func> calls;
    std::vector<std::string> libs {"libigc.a"};
    SrcFile* main;
    std::string m_name;
    std::unordered_map<std::string,SrcFile*> file_table;
    std::unordered_map<std::string,Header*> header_table;
    uint flags;

    bool hasFlag(const std::string&flag) const;
};

struct FileItterator{
    std::vector<SrcFile*> files;
    Directory* dir{};
};

struct HeaderItterator {
    std::vector<Header*> files;
    Directory* dir{};
};

std::string removeExtension(const std::string& filename);
class InfoParser {
public:
    explicit InfoParser(std::vector<Token> tokens);

    static std::map<std::string,std::function<void(InfoParser*,std::vector<std::string>)>> funcs;

    Info* parse();

    std::optional<Func> parseFuncCall();

    HeaderItterator listHeaders(std::string path,const std::string& name);

    FileItterator listFiles(std::string path,const std::string& name);

private:
    [[nodiscard]] std::optional<Token> peak(int count = 0) const{
        if(m_I + count >= m_tokens.size())return {};
        return m_tokens.at(m_I + count);
    }

    Token consume(){
        return m_tokens.at(m_I++);
    }

    Token tryConsume(TokenType type,const std::string& err){
        if(peak().has_value() && peak().value().type == type)return consume();

        std::cerr << err << "\n" << (peak().has_value() ? peak().value().file:peak(-1).value().file) << ": " << (peak().has_value() ? peak().value().line:peak(-1).value().line) << "\n";
        std::cerr << std::endl;
        exit(EXIT_FAILURE);
    }

    std::optional<Token> tryConsume(TokenType type){
        if(peak().has_value() && peak().value().type == type)return consume();
        return {};
    }

    std::vector<Token> m_tokens;
    size_t m_I = 0;
    std::string m_main;
    Info* m_info;
};


#endif //IGEL_COMPILER_INFOPARSER_H
