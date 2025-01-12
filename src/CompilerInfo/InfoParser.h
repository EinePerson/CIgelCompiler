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
#include <map>

#include <llvm/IR/IRBuilder.h>
#include <sys/stat.h>

#include "../Info.h"
#include "../tokenizer.h"

class InfoParser;
struct IgFunction;
struct IgType;
struct Struct;
struct Class;
struct Interface;
struct Token;
struct NodeStmt;
struct InfoSrcFile;
/*namespace llvm {
    template <>
    class IRBuilder;
}*/

struct Func{
    std::vector<std::string> args {};
    std::function<void(InfoParser*, std::vector<std::string>)> func;
};

enum class FunctionState{
    _include,
    _use,
    _inside,
};

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

    std::unordered_map<std::string,IgFunction*> funcs;

    std::unordered_map<std::string,IgType*> nameTypeMap;
    std::unordered_map<std::string,Struct*> structs;
    std::unordered_map<std::string,Class*> classes;
    std::unordered_map<std::string,Interface*> interfaces;

    std::optional<std::pair<llvm::FunctionCallee,bool>> findFunc(std::string name,std::vector<llvm::Type*> types);
    std::optional<IgFunction*> findIgFunc(std::string name, std::vector<llvm::Type*> types);
    std::optional<std::pair<llvm::StructType*,Struct*>> findStruct(std::string name,llvm::IRBuilder<>* builder);
    std::optional<std::pair<llvm::StructType*,Class*>> findClass(std::string name,llvm::IRBuilder<>* builder);
    std::optional<Interface*> findInterface(std::string name);
    std::optional<IgType*> findContained(const std::string & name);
    std::optional<IgType*> findUnmangledContained(const std::string & name);

    Header() = default;

    explicit Header(std::string second);
};

struct SrcFile{
    SrcFile(InfoSrcFile* src);

    SrcFile();

    bool isMain = false;
    bool isGen = false;
    bool isLive = false;
    std::vector<Header*> includes;
    std::vector<SrcFile*> _using;
    std::string dir;
    std::string name;
    //This is with the path
    std::string fullName;
    std::vector<Token> tokens;
    size_t tokenPtr = 0;
    std::vector<NodeStmt*> stmts;
    std::vector<IgType*> types;
    std::vector<std::string> typeNames;
    std::unordered_map<std::string,IgType*> nameTypeMap;
    std::unordered_map<std::string,IgType*> unmangledTypeMap;
    std::unordered_map<std::string,IgFunction*> funcs;
    std::unordered_map<std::string,std::pair<llvm::StructType*,Struct*>> structs;
    std::unordered_map<std::string,std::pair<llvm::StructType*,Class*>> classes;
    std::unordered_map<std::string,Interface*> interfaces;

    std::optional<std::pair<llvm::FunctionCallee,bool>> findFunc(std::string name, std::vector<llvm::Type *> types);
    std::optional<IgFunction*> findIgFunc(std::string name, std::vector<llvm::Type*> types);
    std::optional<std::pair<llvm::StructType*,Struct*>> findStruct(std::string name,llvm::IRBuilder<>* builder);
    std::optional<std::pair<llvm::StructType*,Class*>> findClass(std::string name,llvm::IRBuilder<>* builder);
    std::optional<Interface*> findInterface(std::string name);
    std::optional<IgType*> findContained(std::string name);
    std::optional<IgType*> findUnmangledContained(std::string name);
};

struct Directory{
    explicit Directory(InfoDirectory* dir);

    std::vector<SrcFile*> files;
    std::vector<Header*> headers;
    std::vector<Directory*> sub_dirs;
    std::vector<Directory*> includes;
    std::string name;

    static void genFile(std::string path);
};

/*struct CmpInfo{
    std::vector<SrcFile*> liveFiles;///files which are Just-in-time compiled machine files may not reference those
    std::vector<SrcFile*> files;
    std::vector<Header*> headers;
    std::vector<Directory*> src;
    std::vector<Directory*> include;
    std::vector<Func> calls;
    std::vector<std::string> libs {};
    SrcFile* main = nullptr;
    std::string m_name;
    std::unordered_map<std::string,SrcFile*> file_table;
    std::unordered_map<std::string,Header*> header_table;
    uint flags = 0;

    std::string execDir;

    bool hasFlag(const std::string&flag) const;
};*/

/*struct FileItterator{
    std::vector<SrcFile*> files;
    Directory* dir = nullptr;
};

struct HeaderItterator {
    std::vector<Header*> files;
    Directory* dir = nullptr;
};*/

std::string removeExtension(const std::string& filename);

/*inline std::string getExtension(const std::string &filename) {
    return filename.substr(filename.find_last_of('.') + 1);
}*/
/*class InfoParser {
public:
    explicit InfoParser(std::vector<Token> tokens);

    static std::map<std::string,std::function<void(InfoParser*,std::vector<std::string>)>> funcs;

    CmpInfo* parse();

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
    CmpInfo* m_info;
};*/


#endif //IGEL_COMPILER_INFOPARSER_H
