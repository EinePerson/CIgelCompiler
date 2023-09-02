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

        void genExpr(const NodeExpr* expr) {
            struct ExprVisitor{
                Generator* gen;
                void operator()(const NodeIntLit* intLit){
                    gen->m_output << "   mov rax, " << intLit->int_lit.value.value() << "\n";
                    gen->push("rax");
                }
                void operator()(const NodeExprId* id){
                    if(!gen->m_vars.count(id->id.value.value())){
                        std::cerr << "Undeclared Identifier: " << id->id.value.value() << std::endl;
                        exit(EXIT_FAILURE);
                    }
                    std::stringstream offset;
                    offset << "    QWORD [rsp + " << (gen->m_stackPos - gen->m_vars.at(id->id.value.value()).stackPos - 1) * 8 << "]\n";
                    gen->push(offset.str());
                }
                void operator()(const NodeBinExpr* binExtr) const {
                    throw false;
                }
            };
            ExprVisitor visitor{.gen = this};
            std::visit(visitor,expr->var);
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