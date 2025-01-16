//
// Created by igel on 15.07.24.
//

#ifndef CXX_PARSER_H
#define CXX_PARSER_H
#include <clang-c/CXString.h>
#include <clang/AST/RecursiveASTVisitor.h>
#include <clang/Frontend/FrontendAction.h>
#include <clang/Tooling/Tooling.h>
#include <llvm/Support/CommandLine.h>
#include <llvm/Support/Process.h>
#include <iostream>

#include "../CompilerInfo/InfoParser.h"
#include "../util/Process.h"
#include "../types.h"

namespace CXX_Ext {
    inline void err(const std::string &err) {
        std::cerr << err << std::endl;
        exit(EXIT_FAILURE);
    }
}

class CXX_Parser;
class Consumer;

class ASTVisitor : public clang::RecursiveASTVisitor<ASTVisitor> {
public:
    friend Consumer;

    explicit ASTVisitor(CXX_Parser* parser);

    bool VisitFunctionDecl(clang::FunctionDecl* function_decl);

    bool VisitNamespaceDecl(clang::NamespaceDecl* namespace_decl);

    bool VisitEnumDecl(clang::EnumDecl* enum_decl);
    bool VisitCXXRecordDecl(clang::CXXRecordDecl* record_decl);
    //bool VisitCXXConstructorDecl(clang::CXXConstructorDecl* cnst_decl);

    //bool VisitVarDecl(clang::VarDecl* var_decl);

private:
    Igel::Access convert(clang::AccessSpecifier acc);
    llvm::Type* convert(const clang::Type *type);
    IgFunction* parseFunc(clang::FunctionDecl* method_decl);

    std::optional<IgType*> getTypeFromDecl(clang::DeclContext* cntxt);
    clang::CXXRecordDecl* getOriginalBaseClass(const clang::CXXMethodDecl* method_decl);

    std::optional<Struct*> parseStruct(clang::CXXRecordDecl* decl);
    std::optional<Class*> parseClass(clang::CXXRecordDecl* decl);
    std::optional<Interface*> parseInterface(clang::CXXRecordDecl* decl);


    CXX_Parser* parser;
};

class ASTIndexer : public clang::RecursiveASTVisitor<ASTIndexer> {
public:
    explicit ASTIndexer(CXX_Parser* parser);

    bool VisitNamespaceDecl(clang::NamespaceDecl* namespace_decl);

    bool VisitEnumDecl(clang::EnumDecl* enum_decl);
    bool VisitCXXRecordDecl(clang::CXXRecordDecl* record_decl);

private:
    std::optional<IgType*> getTypeFromDecl(clang::DeclContext* cntxt);

    CXX_Parser* parser;
};

class Consumer : public clang::ASTConsumer {
public:
    explicit Consumer(CXX_Parser* parser);

    void HandleTranslationUnit(clang::ASTContext &context) override{
        indexer.TraverseDecl(context.getTranslationUnitDecl());
        visitor.TraverseDecl(context.getTranslationUnitDecl());
    }
private:
    ASTIndexer indexer;
    ASTVisitor visitor;
    CXX_Parser* parser;
};

class ActionFactory : public clang::tooling::FrontendActionFactory {
public:
    explicit ActionFactory(CXX_Parser* parser);
    std::unique_ptr<clang::FrontendAction> create() override;
private:
    CXX_Parser* parser;
};

class IgFrontendAction final : public clang::ASTFrontendAction {
public:
    explicit IgFrontendAction(CXX_Parser* parser);
    std::unique_ptr<clang::ASTConsumer> CreateASTConsumer(clang::CompilerInstance &CI, llvm::StringRef file) override;

private:
    CXX_Parser* parser;
};

class CXX_Parser {
    friend IgFrontendAction;
    friend ActionFactory;
    friend Consumer;
    friend ASTVisitor;
    friend ASTIndexer;
public:
    //static std::unique_ptr<llvm::LLVMContext> m_contxt;

    explicit CXX_Parser(Header* header,std::vector<std::string> args,std::string dir);

    Header* parseHeader();

private:
    Header* m_header;
    static std::vector<std::string> includes;
    std::vector<std::string> m_args;
    bool m_link;
    std::string m_dir;

public:
    static std::vector<std::string> getClangIncludePaths() {
        if(includes.empty()){

            std::string cmd = "/bin/clang++ -E -x c++ - -v < /dev/null 2>&1";
            std::string result = Process::exec(cmd.c_str());

            std::string line;
            bool found = false;
            std::istringstream iss(result);
            while (std::getline(iss, line)) {
                if (line.find("#include <...> search starts here:") != std::string::npos) {
                    found = true;
                    continue;
                }
                if (found) {
                    if (line.find("End of search list.") != std::string::npos) {
                        break;
                    }
                    // Trim leading spaces and add the path
                    includes.push_back(std::string(line.begin() + line.find_first_not_of(" "), line.end()));
                }
            }
        }
        std::vector<std::string> includePaths;
        for (auto include : includes) {
            includePaths.push_back("-I" + include);
        }
        return includePaths;
    }

    static std::vector<std::string> getIncludes() {
        if(includes.empty())getClangIncludePaths();
        return includes;
    }
};

#endif //CXX_PARSER_H
