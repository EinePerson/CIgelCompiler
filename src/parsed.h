//
// Created by igel on 06.11.23.
//

#pragma once

#include <variant>
#include "tokenizer.h"

struct NodeExpr;
struct NodeStmt;
struct FuncCall;

struct Expression{
    Token token;
};

struct NodeTermIntLit{
    Token int_lit;
};

struct NodeTermId{
    Token id;
};

struct NodeTermParen {
    NodeExpr* expr;
};

struct NodeExprId{
    Token id;
};

struct NodeBinExprAdd{
    NodeExpr* ls;
    NodeExpr* rs;
};

struct NodeBinExprSub{
    NodeExpr* ls;
    NodeExpr* rs;
};

struct NodeBinExprMul{
    NodeExpr* ls;
    NodeExpr* rs;
};

struct NodeBinExprDiv{
    NodeExpr* ls;
    NodeExpr* rs;
};

struct NodeBinAreth{
    NodeExpr* ls;
    NodeExpr* rs;
    TokenType type;
};



struct NodeBinExpr {
    std::variant<NodeBinExprAdd*,NodeBinExprMul*,NodeBinExprSub*,NodeBinExprDiv*,NodeBinAreth*> var;
};

struct NodeTerm{
    std::variant<NodeTermIntLit*,NodeTermId*,NodeTermParen*,FuncCall*> var;
};

struct NodeIntLit{
    Token int_lit;
};

struct NodeExpr{
    std::variant<NodeTerm*,NodeBinExpr*> var;
};

struct FuncCall{
    std::string name;
    std::vector<NodeExpr*> exprs;
};

struct NodeStmtLet {
    char sid;
    bool _signed;
    Token ident;
    NodeExpr* expr;
};

struct NodeStmtExit {
    NodeExpr* expr;
};

struct  NodeStmtScope
{
    std::vector<NodeStmt*> stmts;
};

struct NodeStmtIf{
    std::vector<NodeExpr*> expr;
    std::vector<TokenType> exprCond;
    std::variant<NodeStmtScope*,NodeStmt*> scope;
    std::optional<std::variant<NodeStmtScope*,NodeStmt*>> scope_else;
    std::optional<NodeStmtIf*> else_if;
};

struct NodeStmtReassign{
    Token id;
    std::optional<NodeExpr*> expr;
    TokenType op;
};

struct Function{
    std::string fullName;
    std::string name;
    std::vector<TokenType> paramType;
    std::vector<std::string> paramName;
    TokenType _return;
    NodeStmtScope * scope;
};

struct Return{
    std::optional<NodeExpr*> val;
};

struct NodeStmt{
    std::variant<NodeStmtExit*, NodeStmtLet*,NodeStmtScope*,NodeStmtIf*,NodeStmtReassign*,FuncCall*,Return*> var;
};

struct NodeProgram
{
    std::vector<NodeStmt*> stmts;
    std::vector<Function*> funcs;
};

