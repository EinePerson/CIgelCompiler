#pragma once 

#include <vector>
#include <variant>
#include <iostream>

#include "tokenizer.hpp"
#include "arena.hpp"

struct Expression{
    Token token;
};

struct NodeTermIntLit{
    Token int_lit;
};

struct NodeTermId{
    Token id;
};

struct NodeExprId{
    Token id;
};

struct NodeExpr;

struct NodeBinExprAdd{
    NodeExpr* ls;
    NodeExpr* rs;
};

struct NodeBinExprMul{
    NodeExpr* ls;
    NodeExpr* rs;
};

struct NodeBinExpr {
    NodeBinExprAdd* var;
    //std::variant<NodeBinExprAdd*,NodeBinExprMul*> var;
};

struct NodeTerm{
    std::variant<NodeTermIntLit*,NodeTermId*> var;
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
            }else return {};
        }

        std::optional<NodeExpr*> parseExpr(){
            if(auto term = parseTerm()){
                if(tryConsume(TokenType::plus).has_value()){
                    auto binExpr = m_alloc.alloc<NodeBinExpr>();
                    auto binExpr_add = m_alloc.alloc<NodeBinExprAdd>();
                    auto ls_expr = m_alloc.alloc<NodeExpr>();
                    ls_expr->var = term.value();
                    binExpr_add->ls = ls_expr;
                    if(auto rs = parseExpr()){
                        binExpr_add->rs = rs.value();
                        binExpr->var = binExpr_add;
                        auto expr = m_alloc.alloc<NodeExpr>();
                        expr->var = binExpr;
                        return expr;
                    }else{
                        std::cerr << "Expected expression" << std::endl;
                        exit(EXIT_FAILURE);
                    }
                }else {
                    auto expr = m_alloc.alloc<NodeExpr>();
                    expr->var = term.value();
                    return expr;
                }
            }else return {};
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
            }else if(peak().has_value() && peak().value().type == TokenType::let && peak(1).has_value() && peak(1).value().type == TokenType::id && peak(2).has_value() && peak(2).value().type == TokenType::eq){
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