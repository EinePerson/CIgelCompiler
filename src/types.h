//
// Created by igel on 17.11.23.
//

#ifndef TYPES_H
#define TYPES_H
#include <llvm/IR/IRBuilder.h>

#include "tokenizer.h"
#include "types.h"

struct IgFunction;

//#include "langMain/codeGen/Generator.h"
namespace Types {
    int getSize(TokenType type);
}

llvm::FunctionCallee getFunction(std::string name,std::vector<llvm::Type*> params);
llvm::Type* getType(TokenType type);

struct NodeStmtScope;
struct NodeStmt;

llvm::Value* generateS(std::variant<NodeStmtScope*,NodeStmt*> var,llvm::IRBuilder<>* builder);

inline llvm::CmpInst::Predicate condition(const TokenType type) {
    switch (type) {
        case TokenType::equal: return llvm::CmpInst::ICMP_EQ;
        case TokenType::notequal: return llvm::CmpInst::ICMP_NE;
        case TokenType::big: return llvm::CmpInst::ICMP_SGT;
        case TokenType::small: return llvm::CmpInst::ICMP_SLT;
        case TokenType::bigequal: return llvm::CmpInst::ICMP_SGE;
        case TokenType::smallequal: return llvm::CmpInst::ICMP_SLE;
        default: return llvm::CmpInst::BAD_ICMP_PREDICATE;
    }
}

llvm::Value* generateReassing(llvm::Value* load,llvm::Value* _new,TokenType op,llvm::IRBuilder<>* builder);

struct Node {
    virtual ~Node() = default;

    virtual llvm::Value* generate(llvm::IRBuilder<>* builder) = 0;
};

struct NodeExpr : Node {
    //TODO add info for floating points and sinage
    char floating;
    char _signed;
};
struct NodeStmt;
struct TermFuncCall;
struct NodeTerm : NodeExpr{
    std::optional<NodeTerm*> outer;
};

struct NodeTermIntLit final : NodeTerm{
    Token int_lit;
    char sid;
    llvm::Value* generate(llvm::IRBuilder<>* builder) override {
        int size = Types::getSize(static_cast<TokenType>(sid));
        return  llvm::ConstantInt::get(llvm::IntegerType::get(builder->getContext(),size),std::stoi(int_lit.value.value()),sid <= 3);
    };
};

struct NodeTermId final : NodeTerm{
    Token id;
    llvm::Value* generate(llvm::IRBuilder<>* builder) override;
};

struct NodeTermParen final : NodeTerm {
    NodeExpr* expr = nullptr;
    llvm::Value* generate(llvm::IRBuilder<>* builder) override {

    };
};

struct NodeTermArrayAcces final : NodeTerm{
    Token id;
    std::vector<NodeExpr*> exprs;
    llvm::Value* generate(llvm::IRBuilder<>* builder) override;
};

struct NodeBinExpr : NodeExpr{
    NodeExpr* ls = nullptr;
    NodeExpr* rs = nullptr;
};

struct NodeBinExprAdd final : NodeBinExpr{
    llvm::Value* generate(llvm::IRBuilder<>* builder) override {
        return  builder->CreateAdd(ls->generate(builder),rs->generate(builder));
    };
};

struct NodeBinExprSub final : NodeBinExpr{
    llvm::Value* generate(llvm::IRBuilder<>* builder) override {
        return builder->CreateSub(ls->generate(builder),rs->generate(builder));
    };
};

struct NodeBinExprMul final : NodeBinExpr{
    llvm::Value* generate(llvm::IRBuilder<>* builder) override {
        return builder->CreateMul(ls->generate(builder),rs->generate(builder));
    };
};

struct NodeBinExprDiv final : NodeBinExpr{
    llvm::Value* generate(llvm::IRBuilder<>* builder) override {
        return builder->CreateFDiv(ls->generate(builder),rs->generate(builder));
    };
};

struct NodeBinAreth final : NodeBinExpr{
    TokenType type;
    //TODO add Types
    llvm::Value* generate(llvm::IRBuilder<>* builder) override {
        llvm::AllocaInst* ptr = builder->CreateAlloca(llvm::Type::getInt1Ty(builder->getContext()));
        builder->CreateLoad(ptr->getType(),ptr);
        //return builder->CreateStore(builder->CreateICmp(condition(type),ls->generate(builder),rs->generate(builder)),ptr);
        return  builder->CreateICmp(condition(type),ls->generate(builder),rs->generate(builder));
    };
};

struct TermFuncCall final : NodeTerm{
    std::string name;
    std::vector<NodeExpr*> exprs;
    llvm::Value* generate(llvm::IRBuilder<>* builder) override {
        std::vector<llvm::Type*> params;
        params.reserve(exprs.size());
        std::vector<llvm::Value*> vals;
        vals.reserve(exprs.size());
        for (const auto expr : exprs) {
            llvm::Value* val = expr->generate(builder);
            params.push_back(val->getType());
            vals.push_back(val);
        }
        llvm::FunctionCallee callee = getFunction(name,params);
        llvm::Value* val =  builder->CreateCall(callee,vals);
        return val;
    };
};

struct NodeStmt : Node {
    
};

struct StmtFuncCall final : NodeStmt{
    std::string name;
    std::vector<NodeExpr*> exprs;
    llvm::Value* generate(llvm::IRBuilder<>* builder) override {

    };
};

struct NodeStmtLet : NodeStmt{
    Token ident;
};

struct NodeStmtPirimitiv final : NodeStmtLet {
    char sid;
    bool _signed;
    std::optional<NodeExpr*> expr;
    llvm::Value* generate(llvm::IRBuilder<>* builder) override;
};

struct NodeStmtNew final : NodeStmtLet{
    TokenType id;
    std::vector<NodeExpr*> exprs;
    llvm::Value* generate(llvm::IRBuilder<>* builder) override {
        //TODO add arrays
    };
};

struct NodeStmtArr final : NodeStmtLet{
    char sid;
    bool _signed;
    bool fixed;
    std::vector<uint> size;
    std::optional<NodeStmtNew*> create;
    llvm::Value* generate(llvm::IRBuilder<>* builder) override;
};

struct NodeStmtExit final : NodeStmt{
    NodeExpr* expr;
    llvm::Value* generate(llvm::IRBuilder<>* builder) override;
};

struct  NodeStmtScope final : NodeStmt{
    std::vector<NodeStmt*> stmts;
    llvm::Value* generate(llvm::IRBuilder<>* builder) override {
        llvm::Value* val = nullptr;
        for (const auto stmt : stmts)
            val = stmt->generate(builder);
        return val;
    };
};

struct NodeStmtIf final : NodeStmt{
    std::vector<NodeExpr*> expr;
    std::vector<TokenType> exprCond;
    std::variant<NodeStmtScope*,NodeStmt*> scope;
    std::optional<std::variant<NodeStmtScope*,NodeStmt*>> scope_else;
    std::optional<NodeStmtIf*> else_if;
    llvm::Value* generate(llvm::IRBuilder<>* builder) override;
};

struct NodeStmtReassign final : NodeStmt{
    Token id;
    std::optional<NodeExpr*> expr;
    TokenType op;
    llvm::Value* generate(llvm::IRBuilder<>* builder) override;
};

struct NodeStmtArrReassign final : NodeStmt{
    Token id;
    std::optional<NodeExpr*> expr;
    TokenType op;
    std::vector<NodeExpr*> exprs;
    llvm::Value* generate(llvm::IRBuilder<>* builder) override;
};

struct IgFunction{
    //std::string fullName;
    std::string name;
    std::vector<llvm::Type*> paramType;
    std::vector<std::string> paramName;
    llvm::Type* _return;
    NodeStmtScope * scope;
};

struct Return final : NodeStmt{
    std::optional<NodeExpr*> val;
    llvm::Value* generate(llvm::IRBuilder<>* builder) override;
};

inline llvm::Value* generateS(std::variant<NodeStmtScope*,NodeStmt*> var,llvm::IRBuilder<>* builder) {
    static llvm::Value* val;
    struct Visitor {
        llvm::IRBuilder<>* builder;
        void operator()(NodeStmt* stmt) const {
            val = stmt->generate(builder);
        }
        void operator()(NodeStmtScope* scope) const {
            val = scope->generate(builder);
        }
    };

    Visitor visitor{.builder = builder};
    std::visit(visitor,var);
    return val;
}

#endif //TYPES_H
