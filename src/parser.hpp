#pragma once 

#include <vector>
#include <variant>

#include "tokenizer.hpp"
#include "arena.hpp"

struct Expression{
    Token token;
};

struct NodeExprId{
    Token id;
};

struct NodeExpr;

struct BinExprAdd{
    NodeExpr* ls;
    NodeExpr* rs;
};

struct BinExprMul{
    NodeExpr* ls;
    NodeExpr* rs;
};

struct NodeBinExpr {
    std::variant<BinExprAdd*,BinExprMul*> var;
};

struct NodeIntLit{
    Token int_lit;
};

struct NodeExpr{
    std::variant<NodeIntLit*,NodeExprId*,NodeBinExpr*> var;
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

        std::optional<NodeBinExpr*> parseBinExpr(){
            if(auto ls = parseExpr()){
                auto binExpr = m_alloc.alloc<NodeBinExpr>();
                binExpr->
            }
            else return {};
        }

        std::optional<NodeExpr*> parseExpr(){
            if(peak().has_value() && peak().value().type == TokenType::int_lit){
                NodeIntLit* nodeInt = m_alloc.alloc<NodeIntLit>();
                nodeInt->int_lit = consume();
                auto expr = m_alloc.alloc<NodeExpr>();
                expr->var = nodeInt;
                return expr;
            }
            else if(peak().has_value() && peak().value().type == TokenType::id){
                NodeExprId* nodeId = m_alloc.alloc<NodeExprId>();
                nodeId->id = consume();
                auto expr = m_alloc.alloc<NodeExpr>();
                expr->var = nodeId;
                return expr;
            }else if(auto bin_expr = parseBinExpr()){
                auto expr = m_alloc.alloc<NodeExpr>();
                expr->var = bin_expr.value();
                return expr;
            }
            else return {};
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
                if(peak().has_value() && peak().value().type == TokenType::closeParenth){
                    consume();
                }else{
                    std::cerr << "Expected: ')'" << std::endl;
                    exit(EXIT_FAILURE);
                }
                if(peak().has_value() && peak().value().type == TokenType::semi){
                    consume();
                }else {
                    std::cerr << "Expected ';'" << std::endl;
                    exit(EXIT_FAILURE);
                }
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
                if(peak().has_value() && peak().value().type == TokenType::semi)consume();
                else {
                    std::cerr << "Expected ';'" << std::endl;
                    exit(EXIT_FAILURE);
                }
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

    private:
        const std::vector<Token> m_tokens;
        size_t m_I = 0;
        ArenaAlocator m_alloc;
};