//
// Created by igel on 17.11.23.
//

#ifndef TYPES_H
#define TYPES_H
#include <llvm/IR/IRBuilder.h>

#include "tokenizer.h"
#include "types.h"

struct Var;
struct IgFunction;

//#include "langMain/codeGen/Generator.h"
namespace Types {
    int getSize(TokenType type);
}

std::pair<llvm::FunctionCallee,bool> getFunction(std::string name,std::vector<llvm::Type*> params);
llvm::Type* getType(TokenType type);

struct NodeStmtScope;
struct NodeStmt;

llvm::Type* getType(llvm::Type* type);

llvm::Value* generateS(std::variant<NodeStmtScope*,NodeStmt*> var,llvm::IRBuilder<>* builder);

inline llvm::CmpInst::Predicate condition(const TokenType type,bool _signed) {
    if(_signed) {
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
    switch (type) {
        case TokenType::equal: return llvm::CmpInst::ICMP_EQ;
        case TokenType::notequal: return llvm::CmpInst::ICMP_NE;
        case TokenType::big: return llvm::CmpInst::ICMP_UGT;
        case TokenType::small: return llvm::CmpInst::ICMP_ULT;
        case TokenType::bigequal: return llvm::CmpInst::ICMP_UGE;
        case TokenType::smallequal: return llvm::CmpInst::ICMP_ULE;
        default: return llvm::CmpInst::BAD_ICMP_PREDICATE;
    }
}

llvm::Value* generateReassing(llvm::Value* load,llvm::Value* _new,TokenType op,llvm::IRBuilder<>* builder);

struct Node {
    virtual ~Node() = default;

    virtual llvm::Value* generate(llvm::IRBuilder<>* builder) = 0;

    virtual llvm::Value* generatePointer(llvm::IRBuilder<>* builder) = 0;
};

struct NodeExpr : Node {
    //FLOATING IS STORED IN VALUE* TYPE
    bool floating;
    bool _signed;
};
struct NodeStmt;
struct NodeTermFuncCall;



//BEGIN OF NODE TERMS

struct NodeTerm : NodeExpr{
    std::optional<NodeTerm*> contained;
};

struct NodeTermIntLit final : NodeTerm{
    Token int_lit;
    char sid = 2;
    llvm::Value* generate(llvm::IRBuilder<>* builder) override;

    llvm::Value* generatePointer(llvm::IRBuilder<>* builder) override {
        throw "NotImplementedException";
    };
};

struct NodeTermStringLit final : NodeTerm {
    Token str;
    llvm::Value* generate(llvm::IRBuilder<>* builder) override {
        return  builder->CreateGlobalStringPtr(str.value.value());
    }
    llvm::Value* generatePointer(llvm::IRBuilder<>* builder) override {
        throw "NotImplementedException";
    };
};

struct NodeTermNull final : NodeTerm {
    llvm::Value* generate(llvm::IRBuilder<>* builder) override {
        return llvm::ConstantPointerNull::get(llvm::PointerType::get(builder->getContext(),0));
    }

    llvm::Value* generatePointer(llvm::IRBuilder<>* builder) override {
        throw "NotImplementedException";
    };
};

struct NodeTermId final : NodeTerm{
    Token id;
    //std::vector<std::string> names;
    llvm::Value* generate(llvm::IRBuilder<>* builder) override;
    llvm::Value* generatePointer(llvm::IRBuilder<>* builder) override;
};

struct NodeTermParen final : NodeTerm {
    NodeExpr* expr = nullptr;
    llvm::Value* generate(llvm::IRBuilder<>* builder) override {
        return expr->generate(builder);
    };
    llvm::Value* generatePointer(llvm::IRBuilder<>* builder) override {
        throw "NotImplementedException";
    };
};

struct NodeTermArrayAcces final : NodeTerm{
    Token id;
    std::vector<NodeExpr*> exprs;
    llvm::Value* generate(llvm::IRBuilder<>* builder) override;
    llvm::Value* generatePointer(llvm::IRBuilder<>* builder) override;
};

struct NodeTermStructAcces final : NodeTerm {
    Token id;
    Token acc;
    llvm::Value* generate(llvm::IRBuilder<>* builder) override;
    llvm::Value* generatePointer(llvm::IRBuilder<>* builder) override;
};

struct NodeTermFuncCall final : NodeTerm{
    std::string name;
    std::vector<NodeExpr*> exprs;
    llvm::Value* generate(llvm::IRBuilder<>* builder) override {
        if(contained)contained.value()->generate(builder);
        std::vector<llvm::Type*> params;
        params.reserve(exprs.size());
        std::vector<llvm::Value*> vals;
        vals.reserve(exprs.size());
        for (const auto expr : exprs) {
            llvm::Value* val = expr->generate(builder);
            params.push_back(val->getType());
            vals.push_back(val);
        }
        const std::pair<llvm::FunctionCallee,bool> callee = getFunction(name,params);
        _signed = callee.second;
        llvm::Value* val =  builder->CreateCall(callee.first,vals);
        return val;
    };

    llvm::Value* generatePointer(llvm::IRBuilder<>* builder) override {
        throw "NotImplementedException";
    };
};

struct NodeTermNew final : NodeTerm {
    std::string typeName;
    llvm::Value* generate(llvm::IRBuilder<>* builder) override;

    llvm::Value* generatePointer(llvm::IRBuilder<>* builder) override {
        throw "NotImplementedException";
    };
};



//BEGIN OF BINARY EXPRESIONS

struct NodeBinExpr : NodeExpr{
    NodeExpr* ls = nullptr;
    NodeExpr* rs = nullptr;

    llvm::Value* generatePointer(llvm::IRBuilder<>* builder) override {
        throw "NotImplementedException";
    };
};

struct NodeBinExprAdd final : NodeBinExpr{
    llvm::Value* generate(llvm::IRBuilder<>* builder) override {
        llvm::Value* lsv = ls->generate(builder);
        llvm::Value* rsv = rs->generate(builder);
        char type = 0;
        type += rsv->getType()->isFloatingPointTy()? 1 : 0;
        type += lsv->getType()->isFloatingPointTy()? 2 : 0;
        if(type == 0)return  builder->CreateAdd(lsv,rsv);

        if(type == 2)rsv = rs->_signed ? builder->CreateSIToFP(rsv,lsv->getType()) : builder->CreateUIToFP(rsv,lsv->getType());
        if(type == 1)lsv = ls->_signed ? builder->CreateSIToFP(lsv,rsv->getType()) : builder->CreateUIToFP(lsv,rsv->getType());

        return builder->CreateFAdd(lsv,rsv);
    };
};

struct NodeBinExprSub final : NodeBinExpr{
    llvm::Value* generate(llvm::IRBuilder<>* builder) override {
        llvm::Value* lsv = ls->generate(builder);
        llvm::Value* rsv = rs->generate(builder);
        char type = 0;
        type += rsv->getType()->isFloatingPointTy()? 1 : 0;
        type += lsv->getType()->isFloatingPointTy()? 2 : 0;
        if(type == 0)return  builder->CreateSub(lsv,rsv);

        if(type == 2)rsv = rs->_signed ? builder->CreateSIToFP(rsv,lsv->getType()) : builder->CreateUIToFP(rsv,lsv->getType());
        if(type == 1)lsv = ls->_signed ? builder->CreateSIToFP(lsv,rsv->getType()) : builder->CreateUIToFP(lsv,rsv->getType());

        return builder->CreateFSub(lsv,rsv);
    };
};

struct NodeBinExprMul final : NodeBinExpr{
    llvm::Value* generate(llvm::IRBuilder<>* builder) override {
        llvm::Value* lsv = ls->generate(builder);
        llvm::Value* rsv = rs->generate(builder);
        char type = 0;
        type += rsv->getType()->isFloatingPointTy()? 1 : 0;
        type += lsv->getType()->isFloatingPointTy()? 2 : 0;
        if(type == 0)return  builder->CreateMul(lsv,rsv);

        if(type == 2)rsv = rs->_signed ? builder->CreateSIToFP(rsv,lsv->getType()) : builder->CreateUIToFP(rsv,lsv->getType());
        if(type == 1)lsv = ls->_signed ? builder->CreateSIToFP(lsv,rsv->getType()) : builder->CreateUIToFP(lsv,rsv->getType());

        return builder->CreateFMul(lsv,rsv);
    };
};

struct NodeBinExprDiv final : NodeBinExpr{
    llvm::Value* generate(llvm::IRBuilder<>* builder) override {
        llvm::Value* lsv = ls->generate(builder);
        llvm::Value* rsv = rs->generate(builder);
        char type = 0;
        type += rsv->getType()->isFloatingPointTy()? 1 : 0;
        type += lsv->getType()->isFloatingPointTy()? 2 : 0;
        if(type == 0) {
            if(ls->_signed && rs->_signed)return builder->CreateSDiv(lsv,rsv);
            return builder->CreateUDiv(lsv,rsv);
        }

        if(type == 2)
            rsv = rs->_signed ? builder->CreateSIToFP(rsv,lsv->getType()) : builder->CreateUIToFP(rsv,lsv->getType());
        if(type == 1)
            lsv = ls->_signed ? builder->CreateSIToFP(lsv,rsv->getType()) : builder->CreateUIToFP(lsv,rsv->getType());

        return builder->CreateFDiv(lsv,rsv);
    };
};

struct NodeBinAreth final : NodeBinExpr{
    TokenType type = TokenType::uninit;
    llvm::Value* generate(llvm::IRBuilder<>* builder) override {
        llvm::AllocaInst* ptr = builder->CreateAlloca(llvm::Type::getInt1Ty(builder->getContext()));
        builder->CreateLoad(ptr->getType(),ptr);
        llvm::Value* lsv = ls->generate(builder);
        llvm::Value* rsv = rs->generate(builder);
        char typeI = 0;
        llvm::CmpInst::Predicate lType = condition(type,ls->_signed || rs->_signed);
        typeI += rsv->getType()->isFloatingPointTy()? 1 : 0;
        typeI += lsv->getType()->isFloatingPointTy()? 2 : 0;
        if(typeI == 0) {
            if(ls->_signed && rs->_signed)return builder->CreateSDiv(lsv,rsv);
            return builder->CreateICmp(lType,lsv,rsv);
        }

        if(typeI == 2)
            rsv = rs->_signed ? builder->CreateSIToFP(rsv,lsv->getType()) : builder->CreateUIToFP(rsv,lsv->getType());
        if(typeI == 1)
            lsv = ls->_signed ? builder->CreateSIToFP(lsv,rsv->getType()) : builder->CreateUIToFP(lsv,rsv->getType());

        return builder->CreateFCmp(lType,lsv,rsv);
    };

    llvm::Value* generatePointer(llvm::IRBuilder<>* builder) override {
        throw "NotImplementedException";
    };
};
//BEGIN OF STATEMENT NODES

struct NodeStmt : Node {

    llvm::Value* generatePointer(llvm::IRBuilder<>* builder) override {
        throw "NotImplementedException";
    };
};

struct NodeStmtFuncCall final : NodeStmt{
    NodeTermFuncCall* call = nullptr;
    llvm::Value* generate(llvm::IRBuilder<>* builder) override {
        return call->generate(builder);
    };
};

struct NodeStmtLet : NodeStmt{
    Token id;
    llvm::Type* type = nullptr;
};

struct NodeStmtPirimitiv final : NodeStmtLet {
    char sid = -1;
    bool _signed = true;
    std::optional<NodeExpr*> expr = {};
    llvm::Value* generate(llvm::IRBuilder<>* builder) override;
};

struct NodeStmtNew : NodeStmtLet{
    std::vector<NodeExpr*> exprs = {};
};

struct NodeStmtStructNew final : NodeStmtNew{
    std::string typeName;
    NodeTerm* term = nullptr;
    llvm::Value* generate(llvm::IRBuilder<>* builder) override;
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
    NodeExpr* expr = nullptr;
    llvm::Value* generate(llvm::IRBuilder<>* builder) override;
};

struct  NodeStmtScope final : NodeStmt{
    std::vector<NodeStmt*> stmts;
    llvm::Value* generate(llvm::IRBuilder<>* builder) override /*{
        llvm::Value* val = nullptr;
        for (const auto stmt : stmts)
            val = stmt->generate(builder);
        return val;
    }*/;
};

struct NodeStmtIf final : NodeStmt{
    std::vector<NodeExpr*> expr;
    std::vector<TokenType> exprCond;
    std::variant<NodeStmtScope*,NodeStmt*> scope;
    std::optional<std::variant<NodeStmtScope*,NodeStmt*>> scope_else;
    std::optional<NodeStmtIf*> else_if;
    llvm::Value* generate(llvm::IRBuilder<>* builder) override;
};

struct NodeStmtReassign;

struct NodeStmtFor final : NodeStmt {
    NodeStmtLet* let = nullptr;
    std::vector<NodeExpr*> expr = {};
    std::vector<TokenType> exprCond = {};
    NodeStmtReassign* res = nullptr;
    std::variant<NodeStmtScope*,NodeStmt*> scope;
    llvm::Value* generate(llvm::IRBuilder<>* builder) override;
};

struct NodeStmtWhile final : NodeStmt {
    std::vector<NodeExpr*> expr = {};
    std::vector<TokenType> exprCond = {};
    std::variant<NodeStmtScope*,NodeStmt*> scope;
    llvm::Value* generate(llvm::IRBuilder<>* builder) override;
};

struct NodeStmtCase final : NodeStmt {
    NodeTermIntLit* cond = nullptr;
    bool _default = false;
    llvm::BasicBlock* generate(llvm::IRBuilder<>* builder) override;
};

struct NodeStmtSwitch final : NodeStmt {
    NodeStmtScope* scope {};
    NodeTerm* cond = nullptr;
    llvm::Value* generate(llvm::IRBuilder<>* builder) override;
};

struct NodeStmtReassign final : NodeStmt{
    NodeTerm* id = nullptr;
    NodeExpr* expr = nullptr;
    TokenType op = TokenType::uninit;
    llvm::Value* generate(llvm::IRBuilder<>* builder) override;
};

struct NodeStmtArrReassign final : NodeStmt{
    std::optional<NodeExpr*> expr;
    TokenType op = TokenType::uninit;
    NodeTermArrayAcces* acces = nullptr;
    llvm::Value* generate(llvm::IRBuilder<>* builder) override;
};

struct IgFunction{
    std::string name;
    std::vector<llvm::Type*> paramType;
    std::vector<std::string> paramName;
    std::vector<bool> signage;
    llvm::Type* _return;
    bool returnSigned;
    NodeStmtScope * scope;
};

struct NodeStmtReturn final : NodeStmt{
    std::optional<NodeExpr*> val;
    llvm::Value* generate(llvm::IRBuilder<>* builder) override;
};

struct NodeStmtBreak final : NodeStmt {
    llvm::Value* generate(llvm::IRBuilder<>* builder) override;
};

struct NodeStmtContinue final : NodeStmt {
    llvm::Value* generate(llvm::IRBuilder<>* builder) override;
};



//BEGIN OF TYPES

struct IgType {
    virtual ~IgType() = default;
    std::string name;

    virtual void generateSig(llvm::IRBuilder<>* builder) = 0;
    virtual void generate(llvm::IRBuilder<>* builder) = 0;
};

struct Struct final : IgType {
    std::vector<NodeStmtLet*> vars;
    std::vector<std::string> varIds;
    std::vector<llvm::Type*> types {};

    void generateSig(llvm::IRBuilder<>* builder) override;

    void generate(llvm::IRBuilder<>* builder) override;
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
