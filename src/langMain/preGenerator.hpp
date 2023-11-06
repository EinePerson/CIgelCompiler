#pragma once

#include "generator.hpp"

#include <sstream>
#include <map>
#include <algorithm>
#include <math.h>

int charToReg(char c){
    switch (c)
    {
    case 'b':
        return 0;
        break;
    case 's':
        return 1;
        break;
    case 'L':
        return 3;
        break;
            
    default:
        return 2;
        break;
    }
}


/**
 * This is used to derermin values that are generated after they have been used by the generator
*/
class PreGen{
    public:
        struct Var;
        PreGen(){}

    int preGen(std::variant<NodeStmtScope*,NodeStmt*> scope){
        m_labelI = 0;
        ScopeStmtVisitor visitor({.gen = this});
        std::visit(visitor,scope);
        return m_labelI;
    }

    int genBinExpr(const NodeBinExpr* binExpr,std::optional<std::vector<Var>> vars = {}){
            static int i = -1;
            struct BinExprVisitor{
                PreGen* gen;
                std::optional<std::vector<Var>> vars;
                void operator()(const NodeBinExprSub* sub){
                    int j = gen->genExpr(sub->rs,vars);
                    int k = gen->genExpr(sub->ls,vars);
                    i = std::max(j,k);
                }
                void operator()(const NodeBinExprMul* mul){
                    int j = gen->genExpr(mul->rs,vars);
                    int k = gen->genExpr(mul->ls,vars);
                    i = std::max(j,k);
                }
                void operator()(const NodeBinExprDiv* div){
                    int j = gen->genExpr(div->rs,vars);
                    int k = gen->genExpr(div->ls,vars);
                    i = std::max(j,k);
                }
                void operator()(const NodeBinExprAdd* add){
                    int j = gen->genExpr(add->rs,vars);
                    int k = gen->genExpr(add->ls,vars);
                    i = std::max(j,k);
                }
                void operator()(const NodeBinAreth* areth){
                    int j = gen->genExpr(areth->rs,vars);
                    int k = gen->genExpr(areth->ls,vars);
                    i = std::max(j,k);
                }
            };
            BinExprVisitor visitor{.gen = this,.vars = vars};
            std::visit(visitor,binExpr->var);
            return i;
        }

        int genExpr(const NodeExpr* expr,std::optional<std::vector<Var>> vars = {}) {
            static int i = -1;
            struct ExprVisitor{
                PreGen* gen;
                std::optional<std::vector<Var>> vars;
                void operator()(const NodeTerm* term){
                    i = gen->genTerm(term,vars);
                }
                void operator()(const NodeBinExpr* binExpr) const {
                    i = gen->genBinExpr(binExpr,vars);
                }
            };
            ExprVisitor visitor{.gen = this,.vars = vars};
            std::visit(visitor,expr->var);
            return i;
        }

        int genTerm(const NodeTerm* term,std::optional<std::vector<Var>> vars = {}){
            static int i = -1;
            struct TermVisitor
            {
                PreGen* gen;
                std::optional<std::vector<Var>> vars;
                void operator()(const NodeTermIntLit* intLit) const {
                    std::string s = intLit->int_lit.value.value();
                    i = 2;
                    if(std::isalpha(s.back())){
                        i = charToReg(s.back());
                    }
                }
                void operator()(const NodeTermId* id) const {
                    if(vars.has_value()){
                        auto it = std::find_if(vars.value().cbegin(), vars.value().cend(), [&](const Var& var) {
                            return var.name == id->id.value.value();
                        });
                        if(it == vars.value().cend()){
                            std::cerr << "Undeclared identifier: " << id->id.value.value() << std::endl;
                            exit(EXIT_FAILURE);
                        }
                        const auto& var = *it;
                        i = var.sid;
                    }
                }
                void operator()(const NodeTermParen* paren) const {
                    i = gen->genExpr(paren->expr);
                }
                void operator()(const FuncCall* call){
                }
            };
            TermVisitor visitor({.gen = this,.vars = vars});
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
                    gen->pregen_if(_if);
                }
                void operator()(const NodeStmtReassign* reassign){

                }
                void operator()(const FuncCall* call){
                }
                void operator()(const Return* _return){

                }
            };

            StmtVisitor vis{.gen = this};
            std::visit(vis,stmt->var);
        }

    public:

        struct SizeAble{
            char sid;
            size_t stackPos;
            const bool mem;
            SizeAble(bool m,char sid,size_t stackPos) : mem(m),sid(sid),stackPos(stackPos) {};
        };

        struct RegIns : SizeAble{
            RegIns(char sid,size_t stackPos): SizeAble(false,sid,stackPos) {};
        };

        struct Var : SizeAble{
            Var(char sid,size_t stackPos,bool _signed,std::string name) : SizeAble(true,sid,stackPos), _signed(_signed),name(name) {};
            bool _signed;
            std::string name;
        };

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

    size_t m_labelI;
};