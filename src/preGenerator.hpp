#pragma once

#include "generator.hpp"

#include <sstream>
#include <map>
#include <algorithm>
#include <math.h>

class PreGen{
    public:
        PreGen(NodeProgram prog) : m_prog(prog){}

    int preGen(std::variant<NodeStmtScope*,NodeStmt*> scope){
        m_labelI = 0;
        ScopeStmtVisitor visitor({.gen = this});
        std::visit(visitor,scope);
        return m_labelI;
    }

    int genBinExpr(const NodeBinExpr* binExpr){
            static int i = -1;
            struct BinExprVisitor{
                PreGen* gen;
                void operator()(const NodeBinExprSub* sub){
                    int j = gen->genExpr(sub->rs);
                    int k = gen->genExpr(sub->ls);
                }
                void operator()(const NodeBinExprMul* mul){
                    int j = gen->genExpr(mul->rs);
                    int k = gen->genExpr(mul->ls);
                }
                void operator()(const NodeBinExprDiv* div){
                    int j = gen->genExpr(div->rs);
                    int k = gen->genExpr(div->ls);
                }
                void operator()(const NodeBinExprAdd* add){
                    int j = gen->genExpr(add->rs);
                    int k = gen->genExpr(add->ls);
                }
                void operator()(const NodeBinAreth* areth){
                    int j = gen->genExpr(areth->rs);
                    int k = gen->genExpr(areth->ls);
                }
            };
            BinExprVisitor visitor{.gen = this};
            std::visit(visitor,binExpr->var);
            return i;
        }

        int genExpr(const NodeExpr* expr) {
            static int i = -1;
            struct ExprVisitor{
                PreGen* gen;
                void operator()(const NodeTerm* term){
                    i = gen->genTerm(term);
                }
                void operator()(const NodeBinExpr* binExpr) const {
                    i = gen->genBinExpr(binExpr);
                }
            };
            ExprVisitor visitor{.gen = this};
            std::visit(visitor,expr->var);
            return i;
        }

        int genTerm(const NodeTerm* term){
            static int i = -1;
            struct TermVisitor
            {
                PreGen* gen;
                void operator()(const NodeTermIntLit* intLit) const {

                }
                void operator()(const NodeTermId* id) const {
                }
                void operator()(const NodeTermParen* paren) const {
                    i = gen->genExpr(paren->expr);
                }
            };
            TermVisitor visitor({.gen = this});
            std::visit(visitor,term->var);
            return i;
        }

        void genScope(const NodeStmtScope* scope){
            for(const NodeStmt* stmt : scope->stmts){
                gen_stmt(stmt);
            }
        }

        void gen_stmt(const NodeStmt* stmt) {
            struct StmtVisitor{
                PreGen* gen;
                void operator()(const NodeStmtExit* stmt_exit){

                }
                void operator()(const NodeStmtLet* stmt_let){
                    gen->genExpr(stmt_let->expr);
                }
                void operator()(const NodeStmtScope* scope){
                    gen->genScope(scope);
                }
                void operator()(const NodeStmtIf* _if) const{
                    /*for(int i = 0;i < _if->expr.size() - 1;i++){
                        auto expr = _if->expr.at(i);
                        gen->genExpr(expr);
                        gen->m_labelI++;
                    }*/
                    //auto expr = _if->expr.back();
                    //gen->genExpr(expr);
                    gen->pregen_if(_if);
                }
                void operator()(const NodeStmtReassign* reassign){

                }
            };

            StmtVisitor vis{.gen = this};
            std::visit(vis,stmt->var);
        }

    private:

        void pregen_if(const NodeStmtIf* _if){
            m_labelI++;
            m_labelI++;
                    
            ScopeStmtVisitor vis{.gen = this};
            std::visit(vis,_if->scope);
            if(_if->else_if.has_value()){
                pregen_if(_if->else_if.value());
                m_labelI++;
            }else if(_if->scope_else.has_value()){
                std::visit(vis,_if->scope_else.value());
                m_labelI++;
            }
        };

        struct ScopeStmtVisitor{
            PreGen* gen;
            void operator()(NodeStmt* stmt){
                gen->gen_stmt(stmt);
            }
            void operator()(NodeStmtScope* scope){
                gen->genScope(scope);
            }
        };

    const NodeProgram m_prog;
    size_t m_labelI;

};