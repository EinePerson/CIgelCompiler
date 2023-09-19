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
                std::cerr << "Size has to be at least 1 and not bigger then 8, but is: " << size;
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

        int genBinExpr(const NodeBinExpr* binExpr){
            static int i;
            struct BinExprVisitor{
                Generator* gen;
                void operator()(const NodeBinExprSub* sub){
                    int j = gen->genExpr(sub->rs);
                    int k = gen->genExpr(sub->ls);
                    i = std::max(j,k);
                    gen->m_output << "   mov " << regs[i][0] << ",0\n";
                    gen->pop(j,0);
                    gen->pop(k,1);
                    
                    gen-> m_output << "   sub " << regs[i][0] << ", " << regs[k][1] << "\n";
                    gen->push(i,0);
                }
                void operator()(const NodeBinExprMul* mul){
                    int j = gen->genExpr(mul->rs);
                    int k = gen->genExpr(mul->ls);
                    i = std::max(j,k);
                    gen->m_output << "   mov " << regs[i][0] << ",0\n";
                    gen->pop(j,0);
                    gen->pop(k,1);
                    
                    gen-> m_output << "   mul " << regs[k][1] << "\n";
                    gen->push(i,0);
                }
                void operator()(const NodeBinExprDiv* div){
                    int j = gen->genExpr(div->rs);
                    int k = gen->genExpr(div->ls);
                    i = std::max(j,k);
                    gen->m_output << "   mov " << regs[i][0] << ",0\n";
                    gen->pop(j,0);
                    gen->pop(k,1);
                    
                    gen-> m_output << "   div " << regs[k][1] << "\n";
                    gen->push(i,0);
                }
                void operator()(const NodeBinExprAdd* add){
                    int j = gen->genExpr(add->rs);
                    int k = gen->genExpr(add->ls);
                    i = std::max(j,k);
                    gen->m_output << "   mov " << regs[i][0] << ",0\n";
                    gen->pop(j,0);
                    gen->pop(k,1);
                    
                    gen-> m_output << "   add " << regs[i][0] << ", " << regs[k][1] << "\n";
                    gen->push(i,0);
                }
            };
            BinExprVisitor visitor{.gen = this};
            std::visit(visitor,binExpr->var);
            return i;
        }

        int genExpr(const NodeExpr* expr) {
            static int i = -1;
            struct ExprVisitor{
                Generator* gen;
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
                Generator* gen;

                void operator()(const NodeTermIntLit* intLit) const {
                    std::string s = intLit->int_lit.value.value();
                    i = 2;
                    if(std::isalpha(s.back())){
                        i = charToReg(s.back());
                        s.pop_back();
                    }
                    gen->m_output << "   mov " << regs[i][0] << ", " << s << "\n";
                    gen->push(i,0);
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
                    gen->copFVar(var);
                    i = var.sid;
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
                    gen->pop(3,4);
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

        void copFVar(Var var){
            switch (var.sid)
            {
            case 0:
                m_output << "   mov al,BYTE [rsp - " << var.stackPos << "]\n";
                m_output << "   mov BYTE [rsp - " << m_stackPos << "],al\n";
                break;
            case 1:
                m_output << "   mov ax,WORD [rsp - " << var.stackPos << "]\n";
                m_output << "   mov WORD [rsp - " << m_stackPos << "],ax\n";
                break;
            case 2:
                m_output << "   mov eax,DWORD [rsp - " << var.stackPos << "]\n";
                m_output << "   mov DWORD [rsp - " << m_stackPos << "],eax\n";
                break;
            case 3:
                m_output << "   mov rax,QWORD [rsp - " << var.stackPos << "]\n";
                m_output << "   mov QWORD [rsp - " << m_stackPos << "],rax\n";
                break;
            default:
                break;
            }
            m_stackPos += regIDSize(var.sid);
        }

        /*void pushSized(const int size,const int reg){
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
        }*/

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

        void writeRam(Var var,long val){
            switch (var.sid){
            case 0:
                m_output << "   mov BYTE [rsp - " << m_stackPos - var.stackPos  - regIDSize(var.sid) << "]," << val << "\n";
                break;
            case 1:
                m_output << "   mov WORD [rsp - " << m_stackPos - var.stackPos  - regIDSize(var.sid) << "]," << val << "\n";
                break;
            case 2:
                m_output << "   mov DWORD [rsp - " << m_stackPos - var.stackPos  - regIDSize(var.sid) << "]," << val << "\n";
                break;
            case 3:
                m_output << "   mov QWORD [rsp - " << m_stackPos - var.stackPos  - regIDSize(var.sid) << "]," << val << "\n";
                break;
            
            default:
                break;
            }
        }

        void writeRam(Var var,int reg){
            switch (var.sid){
            case 0:
                m_output << "   mov BYTE [rsp - " << m_stackPos - var.stackPos  - regIDSize(var.sid) << "]," << regs[var.sid][reg] << "\n";
                break;
            case 1:
                m_output << "   mov WORD [rsp - " << m_stackPos - var.stackPos  - regIDSize(var.sid) << "]," << regs[var.sid][reg] << "\n";
                break;
            case 2:
                m_output << "   mov DWORD [rsp - " << m_stackPos - var.stackPos  - regIDSize(var.sid) << "]," << regs[var.sid][reg] << "\n";
                break;
            case 3:
                m_output << "   mov QWORD [rsp - " << m_stackPos - var.stackPos  - regIDSize(var.sid) << "]," << regs[var.sid][reg] << "\n";
                break;
            
            default:
                break;
            }
        }

        void push(int sid,int reg){
            switch (sid){
            case 0:
                m_output << "   mov BYTE [rsp - " << m_stackPos << "]," << regs[sid][reg] << "\n";
                break;
            case 1:
                m_output << "   mov WORD [rsp - " << m_stackPos << "]," << regs[sid][reg] << "\n";
                break;
            case 2:
                m_output << "   mov DWORD [rsp - " << m_stackPos << "]," << regs[sid][reg] << "\n";
                break;
            case 3:
                m_output << "   mov QWORD [rsp - " << m_stackPos << "]," << regs[sid][reg] << "\n";
                break;
            
            default:
                break;
            }
            m_stackPos += regIDSize(sid);
        }

        void pop(int sid,int reg){
            m_stackPos -= regIDSize(sid);
            switch (sid){
            case 0:
                m_output << "   mov " << regs[sid][reg] << ",BYTE [rsp - " << m_stackPos << "]\n";
                break;
            case 1:
                m_output << "   mov " << regs[sid][reg] << ",WORD [rsp - " << m_stackPos << "]\n";
                break;
            case 2:
                m_output << "   mov " << regs[sid][reg] << ",DWORD [rsp - " << m_stackPos << "]\n";
                break;
            case 3:
                m_output << "   mov " << regs[sid][reg] << ",QWORD [rsp - " << m_stackPos << "]\n";
                break;
            
            default:
                break;
            }
        }

        void readRam(Var var,uint reg){
            switch (var.sid){
            case 0:
                m_output << "   mov " << regs[0][reg] << ", BYTE PTR [rsp - " << m_stackPos - var.stackPos  - regIDSize(var.sid) << "]\n";
                break;
            case 1:
                m_output << "   mov " << regs[1][reg] << ", WORD PTR [rsp - " << m_stackPos - var.stackPos  - regIDSize(var.sid) << "]\n";
                break;
            case 2:
                m_output << "   mov " << regs[2][reg] << ", DWORD PTR [rsp - " << m_stackPos - var.stackPos  - regIDSize(var.sid) << "]\n";
                break;
            case 3:
                m_output << "   mov " << regs[3][reg] << ", QWORD PTR [rsp - " << m_stackPos - var.stackPos  - regIDSize(var.sid) << "]\n";
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