#pragma once

#include "parser.hpp"

#include <sstream>
#include <map>
#include <algorithm>
#include <math.h>

int sizeToReg(uint size){
            switch (size)
            {
            case 1:
                return 0;
            case 2:
                return 1;
            case 3:
            case 4:
                return 2;
            case 5:
            case 6:
            case 7:
            case 8:
                return 3;
            default:
                std::cout << "Size has to be at least 1 and not bigger then 8, but is: " << size;
                exit(EXIT_FAILURE);
            }
        }

        int regIDSize(int id){
            switch (id)
            {
            case 0:
                return 1;
            case 1:
                return 2;
            case 2:
                return 4;
            case 3:
                return 8;           
            default:
                break;
            }
            return -1;
        }

        std::string regs[4][16] = {
            {"AL","BL","CL","DL","DIL","SIL","BPL","SPL","R8B","R9B","R10B","R11B","R12B","R13B","R14B","R15B"},
            {"AX","BX","CX","DX","DI","SI","BP","SP","R8W","R9W","R10W","R11W","R12W","R13W","R14W","R15W"},
            {"EAX","EBX","ECX","EDX","EDI","ESI","EBP","ESP","R8D","R9D","R10D","R11D","R12D","R13D","R14D","R15D"},
            {"RAX","RBX","RCX","RDX","RDI","RSI","RBP","RSP","R8","R9","R10","R11","R12","R13","R14","R15"},
        };

class Generator {

struct Var;
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
                    gen->popSized(3,0);
                    gen->popSized(3,1);
                    
                    gen-> m_output << "   sub rax, rbx\n";
                    gen->pushSized(3,0);
                }
                void operator()(const NodeBinExprMul* mul){
                    gen->genExpr(mul->rs);
                    gen->genExpr(mul->ls);
                    gen->popSized(3,0);
                    gen->popSized(3,1);
                    
                    gen-> m_output << "   mul rbx\n";
                    gen->pushSized(3,0);
                }
                void operator()(const NodeBinExprDiv* div){
                    gen->genExpr(div->rs);
                    gen->genExpr(div->ls);
                    gen->popSized(3,0);
                    gen->popSized(3,1);
                    
                    gen-> m_output << "   div rbx\n";
                    gen->pushSized(3,0);
                }
                void operator()(const NodeBinExprAdd* add){
                    gen->genExpr(add->rs);
                    gen->genExpr(add->ls);
                    gen->popSized(3,0);
                    gen->popSized(3,1);
                    
                    gen-> m_output << "   add rax, rbx\n";
                    gen->pushSized(3,0);
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
            static int i = -1;
            struct TermVisitor
            {
                Generator* gen;

                void operator()(const NodeTermIntLit* intLit) const {
                    gen->m_output << "   mov rax, " << intLit->int_lit.value.value() << "\n";
                    gen->pushSized(3,0);
                }
                void operator()(const NodeTermId* id) const {
                    auto it = std::find_if(gen->m_vars.cbegin(), gen->m_vars.cend(), [&](const Var& var) {
                        return var.name == id->id.value.value();
                    });
                    if(it == gen->m_vars.cend()){
                        std::cerr << "Undeclared identifier: " << id->id.value.value() << std::endl;
                        exit(EXIT_FAILURE);
                    }
                    const auto& var = *it;
                    gen->modVar(var);
                    i = 0;
                }
                void operator()(const NodeTermParen* paren) const {
                    gen->genExpr(paren->expr);
                }
            };
            TermVisitor visitor({.gen = this});
            std::visit(visitor,term->var);
        }

        void genScope(const NodeStmtScope* scope){
            beginScope();
            for(const NodeStmt* stmt : scope->stmts){
                gen_stmt(stmt);
            }
            endScope();
        }

        void gen_stmt(const NodeStmt* stmt) {
            struct StmtVisitor{
                Generator* gen;
                void operator()(const NodeStmtExit* stmt_exit){
                    gen->genExpr(stmt_exit->expr);
                    gen->m_output << "   mov rax, 60\n";
                    gen->popSized(3,4);
                    gen->m_output << "   syscall\n";
                }
                void operator()(const NodeStmtLet* stmt_let){
                    auto it = std::find_if(gen->m_vars.cbegin(), gen->m_vars.cend(), [&](const Var& var) {
                        return var.name == stmt_let->ident.value.value();
                    });
                    if (it != gen->m_vars.cend()){
                        std::cerr << "Identifier already used: " << stmt_let->ident.value.value() << std::endl;
                        exit(EXIT_FAILURE);
                    }
                    gen->m_varsBytes += regIDSize(stmt_let->sid);
                    gen->m_vars.push_back(Var{.sid = stmt_let->sid,._signed = stmt_let->_signed,.stackPos = gen->m_stackPos,.name = stmt_let->ident.value.value()});
                    gen->genExpr(stmt_let->expr);
                }
                void operator()(const NodeStmtScope* scope){
                    gen->genScope(scope);
                }
                void operator()(const NodeStmtIf* _if) const{
                    gen->genExpr(_if->expr);
                    gen->pop(8,0);
                    std::string lable = gen->createLabel();
                    gen->m_output << "   test rax, rax \n";
                    gen->m_output << "   jz " << lable << "\n";
                    gen->genScope(_if->scope);
                    gen->m_output << lable << ":\n";
                }
            };

            StmtVisitor vis{.gen = this};
            std::visit(vis,stmt->var);
        }

    private:

        void modVar(Var var){
            long i = m_stackPos - var.stackPos;
            if(i > 0){
                m_output << "   push QWORD [rsp + " << i - regIDSize(var.sid) << "]\n";
                m_stackPos += 8;
            }
            else if(i < 0){
                m_output << "   push QWORD [rsp - " << (i * -1) - regIDSize(var.sid) << "]\n";
                m_stackPos -= 8;
            }
        }

        void pushSized(const int size,const int reg){
            m_output << "   push " << regs[size][reg] << "\n";
            m_stackPos += regIDSize(size);
        }

        void push(const int type,const int reg){
            pushSized(sizeToReg(type),reg);
        }

        void popSized(const int size,const int reg){
            m_output << "   pop " << regs[size][reg] << "\n";
            m_stackPos -= regIDSize(size);
        }

        void pop(const int type,const int reg){
            popSized(sizeToReg(type),reg);
        }

        void beginScope(){
            m_scopes.push_back(m_varsBytes);
        }

        void endScope() {
            size_t i = m_varsBytes - m_scopes.back();
            m_output << "   add rsp, " << i * 8 << "\n";
            m_stackPos -= i;
            for(int j = 0;j < i;j++){
                m_varsBytes -= regIDSize(m_vars.back().sid);
                m_vars.pop_back();
            }
            m_varsBytes -= regIDSize(m_vars.back().sid);
            m_scopes.pop_back();
        }

        std::string createLabel(){
            std::stringstream ss;
            ss << "label" << m_labelI++;
            return ss.str();
        }

        void moveRAM(Var var,char val){
            m_output << "   mov BYTE PTR [rsp - " << m_stackPos - var.stackPos  - regIDSize(var.sid) << "]," << val << "\n";
        }

        void moveRAM(Var var,short val){
            m_output << "   mov WORD PTR [rsp - " << m_stackPos - var.stackPos  - regIDSize(var.sid) << "]," << val << "\n";
        }

        void moveRAM(Var var,int val){
            m_output << "   mov DWORD PTR [rsp - " << m_stackPos - var.stackPos  - regIDSize(var.sid) << "]," << val << "\n";
        }

        void moveRAM(Var var,long val){
            m_output << "   mov QWORD PTR [rsp - " << m_stackPos - var.stackPos  - regIDSize(var.sid) << "]," << val << "\n";
        }

        void moveRAM(Var var,long val,char sid){
            switch (sid){
            case 0:
                moveRAM(var,(char) val);
                break;
            case 1:
                moveRAM(var,(short) val);
                break;
            case 2:
                moveRAM(var,(int) val);
                break;
            case 3:
                moveRAM(var,(long) val);
                break;
            
            default:
                break;
            }
        }

        struct Var{
            char sid;
            bool _signed;
            size_t stackPos;
            std::string name;
        };

        const NodeProgram m_prog;
        std::stringstream m_output;
        size_t m_stackPos = 0;
        size_t m_varsBytes = 0;
        std::vector<Var> m_vars {};
        std::vector<size_t> m_scopes;
        size_t m_labelI = 0;
};