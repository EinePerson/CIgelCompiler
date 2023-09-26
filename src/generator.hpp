#pragma once

#include "parser.hpp"
#include "preGenerator.hpp"

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

        std::string binjump(TokenType type){
            switch (type)
            {
                case TokenType::bigequal:
                    return "JNGE";
                    break;
                case TokenType::equal:
                    return "JNE";
                    break;
                case TokenType::notequal:
                    return "JE";
                    break;
                case TokenType::smallequal:
                    return "JNBE";
                    break;
                case TokenType::small:
                    return "JNL";
                    break;
                case TokenType::big:
                    return "JNG";
                    break;
    
                default:
                    return "";
                    break;
            }
        }

        std::string binnotjump(TokenType type){
            switch (type)
            {
                case TokenType::bigequal:
                    return "JGE";
                    break;
                case TokenType::equal:
                    return "JE";
                    break;
                case TokenType::notequal:
                    return "JNE";
                    break;
                case TokenType::smallequal:
                    return "JBE";
                    break;
                case TokenType::small:
                    return "JL";
                    break;
                case TokenType::big:
                    return "JG";
                    break;
    
                default:
                    return "";
                    break;
            }
        }

class Generator {

    struct Var;
    struct SizeAble;
    public:
        inline explicit Generator(NodeProgram prog) : m_prog(prog){
            m_pre_gen = new PreGen(m_prog);
        }

        std::string gen_prog(){
            m_output << "global _start\n_start:\n";

            m_stmtI = 0;
            while (m_stmtI < m_prog.stmts.size()){
                const NodeStmt* stmt = m_prog.stmts.at(m_stmtI);
                gen_stmt(stmt);
                m_stmtI++;
            }
            m_output << "   mov rax, 60\n";
            m_output << "   mov rdi, 0\n";
            m_output << "   syscall\n";
            return m_output.str();
        }

        int genBinExpr(const NodeBinExpr* binExpr,std::optional<SizeAble> dest = {}){
            static int i = -1;
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
                    
                    gen->m_lables.back() =gen->m_labelI;
                    gen-> m_output << "   cmp " << regs[i][0] << ", " << regs[k][1] << "\n";
                    i = (int) areth->type;
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
                    std::optional<SizeAble> o = RegIns(0,4);
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
                    std::cout << gen->m_pre_gen->preGen(_if->scope) << "A\n";
                    int m = gen->m_labelI;
                    int j = gen->m_pre_gen->preGen(_if->scope) + 1;
                    int k = gen->m_labelI + j;
                    std::cout << k << ":" << gen->m_labelI << "\n";
                    for(int i = 0;i < _if->expr.size() - 1;i++){
                        auto expr = _if->expr.at(i);
                        auto cond = _if->exprCond.at(i);
                        gen->m_lables.push_back(-1);
                        int l = gen->genExpr(expr);
                        if(gen->m_lables.back() == -1){
                            gen->pop(2,0);
                            size_t lable = gen->m_labelI;
                            gen->m_output << "   test rax, rax \n";
                            gen->m_output << "   jz lable" << lable << "\n";
                            gen->m_lables.back() = lable;
                            gen->m_instruction += 4;
                        }else {
                            if(cond == TokenType::_and){
                                gen->m_output << "   " << binjump((TokenType) l) << " lable" << k << "\n";
                            }else if(cond == TokenType::_or){
                                gen->m_output << "   " << binjump((TokenType) l) << " lable" << gen->m_labelI << "\n";
                            }
                        }
                        gen->m_lables.pop_back();
                    }
                    auto expr = _if->expr.back();
                    gen->m_lables.push_back(-1);
                    int l = gen->genExpr(expr);
                    if(gen->m_lables.back() == -1){
                        gen->pop(2,0);
                        size_t lable = gen->m_labelI;
                        gen->m_output << "   test rax, rax \n";
                        gen->m_output << "   jz lable" << lable << "\n";
                        gen->m_lables.back() = lable;
                        gen->m_instruction += 4;
                    }else {
                        if(_if->exprCond.size() > 0 && _if->exprCond.back() == TokenType::_and){
                            gen->m_output << "   " << binnotjump((TokenType) l) << " lable" << gen->m_labelI << "\n";
                        }else{
                            gen->m_output << "   " << binnotjump((TokenType) l) << " lable" << gen->m_labelI << "\n";
                        }
                    }
                    gen->m_lables.pop_back();
                    
                    gen->m_output << "   jmp lable" << k << "\n";
                    gen->m_output << "lable" << gen->m_labelI << ":\n";
                    gen->m_labelI++;
                    
                    ScopeStmtVisitor vis{.gen = gen};
                    std::visit(vis,_if->scope);
                    gen->m_output << "lable" << gen->m_labelI << ":\n";
                    gen->m_labelI++;
                    std::cout << k << ":" << gen->m_labelI << ":" << m << "\n";
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
            m_output << "   mov " << regIDSize(var.sid) << " [rsp - " << m_stackPos - var.stackPos  - regIDSize(var.sid) << "]," << val << "\n";
            m_instruction += 4;
        }

        void readRam(SizeAble var,int reg){
            m_output << "   mov " << regs[var.sid][reg] << "," << regIDSize(var.sid) << " [rsp - " << m_stackPos - var.stackPos  - regIDSize(var.sid) << "]\n";
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
                            m_output << "   mov " << sidType(dest.value().sid) << " [rsp - " << dest.value().stackPos << "]," << regs[dest.value().sid][0] << "\n";
                            m_instruction += 4;
                        }else{
                            m_output << "   mov " << regs[dest.value().sid][dest.value().stackPos] << "," << regs[dest.value().sid][0] << "\n";
                            m_instruction += 2;
                        }
            }else push(i,0);
        }

        void push(int sid,int reg){
            m_stackPos += regIDSize(sid);
            m_output << "   mov "<< sidType(sid) <<" [rsp - " << m_stackPos << "]," << regs[sid][reg] << "\n";
            m_varsI++;
            
            m_instruction += 4;
        }
        
        void copFVar(Var var){
            m_stackPos += regIDSize(var.sid);
            m_output << "   mov " << regs[var.sid][0] << ","<< sidType(var.sid) <<" [rsp - " << var.stackPos << "]\n";
            m_output << "   mov "<< sidType(var.sid) <<" [rsp - " << m_stackPos << "]," << regs[var.sid][0] << "\n";
            m_varsI++;
            m_instruction += 8;
        }

        void pop(int sid,int reg){
            m_varsI--;
            m_output << "   mov " << regs[sid][reg] << ","<< sidType(sid) <<" [rsp - " << m_stackPos << "]\n";
            m_stackPos -= regIDSize(sid);
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
        size_t m_stmtI;
        size_t m_instruction = 0x401000;
        PreGen* m_pre_gen;
};