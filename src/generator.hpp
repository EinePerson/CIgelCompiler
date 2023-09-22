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

        std::string sidType(char sid){
            switch (sid)
            {
            case 0:
                return "BYTE";
                break;
            case 1:
                return "WORD";
                break;
            case 2:
                return "DWORD";
                break;
            case 3:
                return "QWORD";
                break;
            default:
                std::cerr << "Unkown sid:" << sid << std::endl;
                exit(EXIT_FAILURE);
                break;
            }
        }

class Generator {

    struct Var;
    struct SizeAble;
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
            std::cout << m_instruction << "A\n";
            return m_output.str();
        }

        int genBinExpr(const NodeBinExpr* binExpr,std::optional<SizeAble> dest = {}){
            static int i;
            struct BinExprVisitor{
                Generator* gen;
                std::optional<SizeAble> dest;
                void operator()(const NodeBinExprSub* sub){
                    int j = gen->genExpr(sub->rs);
                    int k = gen->genExpr(sub->ls);
                    i = std::max(j,k);
                    if(dest.has_value())i = std::max((char) i,dest.value().sid);
                    gen->m_output << "   mov " << regs[i][0] << ",0\n";
                    gen->pop(j,0);
                    gen->pop(k,1);
                    gen-> m_output << "   sub " << regs[i][0] << ", " << regs[k][1] << "\n";
                    gen->push(dest,i);
                    gen->m_instruction += 7;
                }
                void operator()(const NodeBinExprMul* mul){
                    int j = gen->genExpr(mul->rs);
                    int k = gen->genExpr(mul->ls);
                    i = std::max(j,k);
                    if(dest.has_value())i = std::max((char) i,dest.value().sid);
                    gen->m_output << "   mov " << regs[i][0] << ",0\n";
                    gen->pop(j,0);
                    gen->pop(k,1);
                    
                    gen-> m_output << "   mul " << regs[k][1] << "\n";
                    gen->push(dest,i);
                    gen->m_instruction += 7;
                }
                void operator()(const NodeBinExprDiv* div){
                    int j = gen->genExpr(div->rs);
                    int k = gen->genExpr(div->ls);
                    i = std::max(j,k);
                    if(dest.has_value())i = std::max((char) i,dest.value().sid);
                    gen->m_output << "   mov " << regs[i][0] << ",0\n";
                    gen->pop(j,0);
                    gen->pop(k,1);
                    
                    gen-> m_output << "   div " << regs[k][1] << "\n";
                    gen->push(dest,i);
                    gen->m_instruction += 7;
                }
                void operator()(const NodeBinExprAdd* add){
                    int j = gen->genExpr(add->rs);
                    int k = gen->genExpr(add->ls);
                    i = std::max(j,k);
                    if(dest.has_value())i = std::max((char) i,dest.value().sid);
                    gen->m_output << "   mov " << regs[i][0] << ",0\n";
                    gen->pop(j,0);
                    gen->pop(k,1);
                    
                    gen-> m_output << "   add " << regs[i][0] << ", " << regs[k][1] << "\n";
                    gen->push(dest,i);
                    gen->m_instruction += 7;
                }
                void operator()(const NodeBinAreth* areth){
                    int j = gen->genExpr(areth->rs);
                    int k = gen->genExpr(areth->ls);
                    i = std::max(j,k);
                    if(dest.has_value())i = std::max((char) i,dest.value().sid);
                    gen->m_output << "   mov " << regs[i][0] << ",0\n";
                    gen->pop(j,0);
                    gen->pop(k,1);
                    
                    gen->m_lables.push_back(gen->m_labelI++);
                    gen-> m_output << "   cmp " << regs[i][0] << ", " << regs[k][1] << "\n";
                    gen->m_output << "   " << binjump(areth->type) << " lable" << gen->m_lables.back() << "\n";
                    gen->m_instruction += 9;
                }
            };
            BinExprVisitor visitor{.gen = this,.dest = dest};
            std::visit(visitor,binExpr->var);
            return i;
        }

        int genExpr(const NodeExpr* expr,std::optional<SizeAble> dest = {}) {
            static int i = -1;
            struct ExprVisitor{
                Generator* gen;
                std::optional<SizeAble> dest;
                void operator()(const NodeTerm* term){
                    i = gen->genTerm(term,dest);
                }
                void operator()(const NodeBinExpr* binExpr) const {
                    i = gen->genBinExpr(binExpr,dest);
                }
            };
            ExprVisitor visitor{.gen = this,.dest = dest};
            std::visit(visitor,expr->var);
            return i;
        }

        int genTerm(const NodeTerm* term,std::optional<SizeAble> dest = {}){
            static int i = -1;
            struct TermVisitor
            {
                Generator* gen;
                std::optional<SizeAble> dest;
                void operator()(const NodeTermIntLit* intLit) const {
                    std::string s = intLit->int_lit.value.value();
                    i = 2;
                    if(std::isalpha(s.back())){
                        i = charToReg(s.back());
                        s.pop_back();
                    }
                    gen->m_output << "   mov " << regs[i][0] << ", " << s << "\n";
                    gen->push(dest,i);
                    gen->m_instruction += 5;
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
                    gen->push(var,dest);
                    i = var.sid;
                }
                void operator()(const NodeTermParen* paren) const {
                    i = gen->genExpr(paren->expr,dest);
                }
            };
            TermVisitor visitor({.gen = this,.dest = dest});
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
                    std::optional<SizeAble> o = RegIns(3,4);
                    gen->genExpr(stmt_exit->expr,o);
                    gen->m_output << "   mov rax, 60\n";
                    gen->m_output << "   syscall\n";
                    gen->m_instruction += 7;
                }
                void operator()(const NodeStmtLet* stmt_let){
                    auto it = std::find_if(gen->m_vars.cbegin(), gen->m_vars.cend(), [&](const Var& var) {
                        return var.name == stmt_let->ident.value.value();
                    });
                    if (it != gen->m_vars.cend()){
                        std::cerr << "Identifier already used: " << stmt_let->ident.value.value() << std::endl;
                        exit(EXIT_FAILURE);
                    }
                    gen->m_varsI++;
                    gen->m_stackPos += regIDSize(stmt_let->sid);
                    Var var(stmt_let->sid,gen->m_stackPos,stmt_let->_signed,stmt_let->ident.value.value());
                    gen->m_vars.push_back(var);
                    gen->genExpr(stmt_let->expr,var);
                }
                void operator()(const NodeStmtScope* scope){
                    gen->genScope(scope);
                }
                void operator()(const NodeStmtIf* _if) const{
                    int i = gen->genExpr(_if->expr);
                    if(i == -1){
                        gen->pop(8,0);
                        
                        std::string lable = gen->createLabel();
                        gen->m_output << "   test rax, rax \n";
                        gen->m_output << "   jz " << lable << "\n";
                        ScopeStmtVisitor vis{.gen = gen};
                        std::visit(vis,_if->scope);
                        gen->m_output << lable << ":\n";
                        //Check if 6 or 4
                        gen->m_instruction += 4;
                    }
                    
                    ScopeStmtVisitor vis{.gen = gen};
                    std::visit(vis,_if->scope);
                    gen->m_output << "lable" << gen->m_lables.back() << ":\n";
                    //Check if is instruction
                    //gen->m_instruction += 2;
                    gen->m_lables.pop_back();
                }
            };

            StmtVisitor vis{.gen = this};
            std::visit(vis,stmt->var);
        }

    private:

        struct ScopeStmtVisitor{
            Generator* gen;
            void operator()(NodeStmt* stmt){
                gen->gen_stmt(stmt);
            }
            void operator()(NodeStmtScope* scope){
                gen->genScope(scope);
            }
        };

        void beginScope(){
            Scope scope{.stack = m_stackPos,.var = m_varsI};
            m_scopes.push_back(scope);
        }

        void endScope() {
            Scope scope = m_scopes.back();
            m_stackPos -= scope.stack;
            size_t i = m_varsI - scope.var;
            for(int j = 0;j < i;j++){
                m_vars.pop_back();
            }
            m_varsI -= i;
            m_scopes.pop_back();
        }

        std::string createLabel(){
            std::stringstream ss;
            ss << "lable" << m_labelI++;
            return ss.str();
        }

        void writeRam(SizeAble var,long val){
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
            m_instruction += 4;
        }

        void readRam(SizeAble var,int reg){
            switch (var.sid){
            case 0:
                m_output << "   mov " << regs[var.sid][reg] << ",BYTE [rsp - " << m_stackPos - var.stackPos  - regIDSize(var.sid) << "]\n";
                break;
            case 1:
                m_output << "   mov " << regs[var.sid][reg] << ",WORD [rsp - " << m_stackPos - var.stackPos  - regIDSize(var.sid) << "]\n";
                break;
            case 2:
                m_output << "   mov " << regs[var.sid][reg] << ",DWORD [rsp - " << m_stackPos - var.stackPos  - regIDSize(var.sid) << "],\n";
                break;
            case 3:
                m_output << "   mov " << regs[var.sid][reg] << ",QWORD [rsp - " << m_stackPos - var.stackPos  - regIDSize(var.sid) << "],\n";
                break;
            
            default:
                break;
            }
            m_instruction += 4;
        }

        void push(Var var,std::optional<SizeAble> dest){
            if(dest.has_value() && dest.value().mem == false){
                if(dest.value().sid > var.sid){
                    m_output << "   movsx " << regs[dest.value().sid][dest.value().stackPos] << ", " << sidType(var.sid) << " [rsp - " << var.stackPos << "]\n";
                    m_instruction += 5;
                }else{
                    m_output << "   mov " << regs[var.sid][dest.value().stackPos] << ", " << sidType(var.sid) << " [rsp - " << var.stackPos << "]\n";
                    m_instruction += 4;
                }
            }else copFVar(var);
        }

        void push(std::optional<SizeAble> dest,char i){
            if(dest.has_value()){
                        if(dest.value().mem){
                            m_output << "  mov " << sidType(dest.value().sid) << "[rsp - " << dest.value().stackPos << "]," << regs[dest.value().sid][0] << "\n";
                            m_instruction += 4;
                        }else{
                            m_output << "   mov " << regs[dest.value().sid][dest.value().stackPos] << "," << regs[dest.value().sid][0] << "\n";
                            m_instruction += 2;
                        }
            }else push(i,0);
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
            m_varsI++;
            m_stackPos += regIDSize(sid);
            m_instruction += 4;
        }
        
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
            m_varsI++;
            m_stackPos += regIDSize(var.sid);
            m_instruction += 8;
        }

        void pop(int sid,int reg){
            m_varsI--;
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
            m_instruction += 4;
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
            m_instruction += 4;
        }

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

        struct Scope{
            size_t stack;
            size_t var;
        };

        const NodeProgram m_prog;
        std::stringstream m_output;
        size_t m_stackPos = 0;
        size_t m_varsI = 0;
        std::vector<Var> m_vars {};
        std::vector<Scope> m_scopes;
        std::vector<int> m_lables;
        size_t m_labelI = 0;
        size_t m_instruction = 0;
};