#pragma once 

#include <vector>
#include <variant>
#include <iostream>

#include "tokenizer.hpp"
#include "arena.hpp"

struct NodeExpr;

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

struct NodeBinExpr {
    std::variant<NodeBinExprAdd*,NodeBinExprMul*,NodeBinExprSub*,NodeBinExprDiv*> var;
};

struct NodeTerm{
    std::variant<NodeTermIntLit*,NodeTermId*,NodeTermParen*> var;
};

struct NodeIntLit{
    Token int_lit;
};

struct NodeExpr{
    std::variant<NodeTerm*,NodeBinExpr*> var;
};

struct NodeStmtLet {
    Token ident;
    NodeExpr* expr;
};

struct NodeStmtExit {
    NodeExpr* expr;
};

struct NodeStmt{
    std::variant<NodeStmtExit*, NodeStmtLet*> var;
};

struct NodeProgram
{
    std::vector<NodeStmt*> stmts;
};


class Parser{
    public:
        inline explicit Parser(std::vector<Token> tokens) : m_tokens(std::move(tokens)),m_alloc(1024 * 1024 * 4){
            
        }

        std::optional<NodeTerm*> parseTerm(){
            if(auto int_lit = tryConsume(TokenType::int_lit)){
                auto nodeInt = m_alloc.alloc<NodeTermIntLit>();
                nodeInt->int_lit = int_lit.value();
                auto expr = m_alloc.alloc<NodeTerm>();
                expr->var = nodeInt;
                return expr;
            }
            else if(auto id = tryConsume(TokenType::id)){
                auto* nodeId = m_alloc.alloc<NodeTermId>();
                nodeId->id = id.value();
                auto expr = m_alloc.alloc<NodeTerm>();
                expr->var = nodeId;
                return expr;
            }else if(auto paren = tryConsume(TokenType::openParenth)){
                auto expr = parseExpr();
                if(!expr.has_value()){
                    std::cerr << "Expected Expression" << std::endl;
                    exit(EXIT_FAILURE);
                }
                tryConsume(TokenType::closeParenth,"Expected ')'");
                auto term_paren = m_alloc.alloc<NodeTermParen>();
                term_paren->expr = expr.value();
                auto term = m_alloc.alloc<NodeTerm>();
                term->var = term_paren;
                return term;
            }else return {};
        }

        std::optional<NodeExpr*> parseExpr(int minPrec = 0){
            std::optional<NodeTerm*> termLs = parseTerm();
            if(!termLs.has_value())return {};
            auto exprLs = m_alloc.alloc<NodeExpr>();
            exprLs->var = termLs.value();

            while(true){
                std::optional<Token> token = peak();
                std::optional<int> precI;
                if(token.has_value()){
                    precI = prec(token->type);
                    if(!precI.has_value() || precI < minPrec)break;
                }else break;
                Token op = consume();
                int nMinPrec = precI.value() + 1;
                auto exprR = parseExpr(nMinPrec);
                if(!exprR.has_value()){
                    std::cerr << "Unable to parse" << std::endl;
                    exit(EXIT_FAILURE);
                }
                auto expr = m_alloc.alloc<NodeBinExpr>();
                auto exprLs2 = m_alloc.alloc<NodeExpr>();
                if(op.type == TokenType::plus){
                    auto add = m_alloc.alloc<NodeBinExprAdd>();
                    exprLs2->var = exprLs->var;
                    add->ls = exprLs2;
                    add->rs = exprR.value();
                    expr->var = add;
                }else if(op.type == TokenType::mul){
                    auto mul = m_alloc.alloc<NodeBinExprMul>();
                    exprLs2->var = exprLs->var;
                    mul->ls = exprLs2;
                    mul->rs = exprR.value();
                    expr->var = mul;
                }else if(op.type == TokenType::div){
                    auto div = m_alloc.alloc<NodeBinExprDiv>();
                    exprLs2->var = exprLs->var;
                    div->ls = exprLs2;
                    div->rs = exprR.value();
                    expr->var = div;
                }else if(op.type == TokenType::sub){
                    auto sub = m_alloc.alloc<NodeBinExprSub>();
                    exprLs2->var = exprLs->var;
                    sub->ls = exprLs2;
                    sub->rs = exprR.value();
                    expr->var = sub;
                }else{
                    std::cerr << "Unkown operator" << std::endl;
                    exit(EXIT_FAILURE);
                }
                exprLs->var = expr;
            }

            return exprLs;
        }

        std::optional<NodeStmt*> parseStmt(){
            if(peak().value().type == TokenType::_exit && peak(1).has_value() && peak(1).value().type == TokenType::openParenth){
                consume();
                consume();
                NodeStmtExit* stmt = m_alloc.alloc<NodeStmtExit>();
                if(auto nodeExpr = parseExpr()){
                    stmt->expr = nodeExpr.value();
                }else{
                    std::cerr << "Invalid Expression" << std::endl;
                    exit(EXIT_FAILURE);
                }
                tryConsume(TokenType::closeParenth, "Expected `)`");
                tryConsume(TokenType::semi,"Expected ';'");
                auto stmtR = m_alloc.alloc<NodeStmt>();
                stmtR->var = stmt;
                return stmtR;
            }else if(peak().has_value() && peak().value().type == TokenType::_long && peak(1).has_value() && peak(1).value().type == TokenType::id && peak(2).has_value() && peak(2).value().type == TokenType::eq){
                consume();
                auto stmt = m_alloc.alloc<NodeStmtLet>();
                stmt->ident = consume();
                consume();
                if(auto expr = parseExpr())stmt->expr = expr.value();
                else {
                    std::cerr << "Unexpected expression" << std::endl;
                    exit(EXIT_FAILURE);
                }
                tryConsume(TokenType::semi,"Expected ';'");
                auto stmtR = m_alloc.alloc<NodeStmt>();
                stmtR->var = stmt;
                return stmtR;
            } else {
                return {};
            }
        }

        std::optional<NodeProgram> parseProg(){
            NodeProgram prog;
            while ((peak().has_value()))
            {
                if(auto stmt = parseStmt()){
                    prog.stmts.push_back(stmt.value());
                }else {
                    std::cerr << "Invalid Expression" << std::endl;
                    exit(EXIT_FAILURE);
                }
            }
            return prog;
        }

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
                std::cerr << err << std::endl;
                exit(EXIT_FAILURE);
            }
        }

        inline std::optional<Token> tryConsume(TokenType type){
            if(peak().has_value() && peak().value().type == type)return consume();
            else return {};
        }

    private:
        const std::vector<Token> m_tokens;
        size_t m_I = 0;
        ArenaAlocator m_alloc;
};