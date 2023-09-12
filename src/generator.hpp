#pragma once

#include "parser.hpp"

#include <sstream>
#include <unordered_map>

class Generator {
    public:
        inline explicit Generator(NodeProgram prog) : m_prog(prog){}

        std::string gen_prog(){
        m_output << "global _start\n_start:\n";

        for (const NodeStmt* stmt : m_prog.stmts) {
            gen_stmt(stmt);
        }

        m_output << "   mov rax, 60\n";
        m_output << "   mov rdi, 0\n";
        m_output << "   syscall\n";
        return m_output.str();
        }

        void genBinExpr(const NodeBinExpr* binExpr){
            struct BinExprVisitor{
                Generator* gen;
                void operator()(const NodeBinExprSub* sub){
                    gen->genExpr(sub->rs);
                    gen->genExpr(sub->ls);
                    gen->pop("rax");
                    gen->pop("rbx");
                    
                    gen-> m_output << "   sub rax, rbx\n";
                    gen->push("rax");
                }
                void operator()(const NodeBinExprMul* mul){
                    gen->genExpr(mul->rs);
                    gen->genExpr(mul->ls);
                    gen->pop("rax");
                    gen->pop("rbx");
                    
                    gen-> m_output << "   mul rbx\n";
                    gen->push("rax");
                }
                void operator()(const NodeBinExprDiv* div){
                    gen->genExpr(div->rs);
                    gen->genExpr(div->ls);
                    gen->pop("rax");
                    gen->pop("rbx");
                    
                    gen-> m_output << "   div rbx\n";
                    gen->push("rax");
                }
                void operator()(const NodeBinExprAdd* add){
                    gen->genExpr(add->rs);
                    gen->genExpr(add->ls);
                    gen->pop("rax");
                    gen->pop("rbx");
                    
                    gen-> m_output << "   add rax, rbx\n";
                    gen->push("rax");
                }
            };
            BinExprVisitor visitor{.gen = this};
            std::visit(visitor,binExpr->var);
        }

        void genExpr(const NodeExpr* expr) {
            struct ExprVisitor{
                Generator* gen;
                void operator()(const NodeTerm* term){
                    gen->genTerm(term);
                }
                void operator()(const NodeBinExpr* binExpr) const {
                    gen->genBinExpr(binExpr);
                }
            };
            ExprVisitor visitor{.gen = this};
            std::visit(visitor,expr->var);
        }

        void genTerm(const NodeTerm* term){
            struct TermVisitor
            {
                Generator* gen;

                void operator()(const NodeTermIntLit* intLit) const {
                    gen->m_output << "   mov rax, " << intLit->int_lit.value.value() << "\n";
                    gen->push("rax");
                }
                void operator()(const NodeTermId* id) const {
                    if(!gen->m_vars.count(id->id.value.value())){
                        std::cerr << "Undeclared Identifier: " << id->id.value.value() << std::endl;
                        exit(EXIT_FAILURE);
                    }
                    std::stringstream offset;
                    offset << "    QWORD [rsp + " << (gen->m_stackPos - gen->m_vars.at(id->id.value.value()).stackPos - 1) * 8 << "]\n";
                    gen->push(offset.str());
                }
                void operator()(const NodeTermParen* paren) const {
                    gen->genExpr(paren->expr);
                }
            };
            TermVisitor visitor({.gen = this});
            std::visit(visitor,term->var);
        }

        void gen_stmt(const NodeStmt* stmt) {
            struct StmtVisitor{
                Generator* gen;
                void operator()(const NodeStmtExit* stmt_exit){
                    gen->genExpr(stmt_exit->expr);
                    gen->m_output << "   mov rax, 60\n";
                    gen->pop("rdi");
                    gen->m_output << "\n" << "   syscall\n";
                }
                void operator()(const NodeStmtLet* stmt_let){
                    if (gen->m_vars.count(stmt_let->ident.value.value())){
                        std::cerr << "Identifier already used: " << stmt_let->ident.value.value() << std::endl;
                        exit(EXIT_FAILURE);
                    }
                    gen->m_vars.insert({stmt_let->ident.value.value(),Var{.stackPos = gen->m_stackPos}});
                    gen->genExpr(stmt_let->expr);
                }
            };

            StmtVisitor vis{.gen = this};
            std::visit(vis,stmt->var);
        }

    private:

        void push(const std::string& reg){
            m_output << "   push " << reg << "\n";
            m_stackPos++;
        }

        void pop(const std::string& reg){
            m_output << "   pop " << reg << "\n";
            m_stackPos--;
        }

        struct Var{
            size_t stackPos;

        };

        const NodeProgram m_prog;
        std::stringstream m_output;
        size_t m_stackPos = 0;
        std::unordered_map<std::string,Var> m_vars {};
};