#pragma once

#include "parser.hpp"
#include "preGenerator.hpp"

#include <sstream>
#include <map>
#include <math.h>
#include <unordered_map>

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
        case 4:
            return 1;
        case 5:
            return 2;
        case 6:
            return 4;
        case 7:
            return 8;           
        default:
            break;
    }
    return -1;
}

std::string regs[4][16] = {
    {"AL" ,"BL" ,"CL" ,"DL" ,"DIL","SIL","BPL","SPL","R8B","R9B","R10B","R11B","R12B","R13B","R14B","R15B"},
    {"AX" ,"BX" ,"CX" ,"DX" ,"DI" ,"SI" ,"BP" ,"SP" ,"R8W","R9W","R10W","R11W","R12W","R13W","R14W","R15W"},
    {"EAX","EBX","ECX","EDX","EDI","ESI","EBP","ESP","R8D","R9D","R10D","R11D","R12D","R13D","R14D","R15D"},
    {"RAX","RBX","RCX","RDX","RDI","RSI","RBP","RSP","R8" ,"R9" ,"R10" ,"R11" ,"R12" ,"R13" ,"R14" ,"R15"},
};

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
    case 4:
        return "BYTE";
        break;
    case 5:
        return "WORD";
        break;
    case 6:
        return "DWORD";
        break;
    case 7:
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

std::string reg(int i,int j){
    return regs[i % 4][j];
}

class Generator {

    public:
        inline explicit Generator(NodeProgram prog) : m_prog(prog){
            m_pre_gen = new PreGen(m_prog);
        }

        std::string gen_prog(){
            m_output << "section .text\n     global _start\n_start:\npush rbp\nmov rbp,rsp\n";

            m_funcs.reserve(m_prog.funcs.size());
            m_vars.push_back({});
            m_stackPos.push_back(0);
            for(auto func : m_prog.funcs){
                m_funcs.insert(std::pair(func->name,func));
            }
            m_stmtI = 0;
            while (m_stmtI < m_prog.stmts.size()){
                const NodeStmt* stmt = m_prog.stmts.at(m_stmtI);
                gen_stmt(stmt);
                m_stmtI++;
            }
            m_output << "pop rbp\n";
            m_output << "   mov rax, 60\n";
            m_output << "   mov rdi, 0\n";
            m_output << "   syscall\n";
            auto value_selector = [](auto pair){return pair.second;};
            std::vector<Function*> funcs(m_funcs.size());
            transform(m_funcs.begin(), m_funcs.end(), funcs.begin(), value_selector);

            for(auto func : funcs){
                genfunc(func);
            }

            return m_output.str();
        }

        

        int genBinExpr(const NodeBinExpr* binExpr,std::optional<PreGen::PreGen::SizeAble> dest = {}){
            static int i = -1;
            struct BinExprVisitor{
                Generator* gen;
                std::optional<PreGen::SizeAble> dest;
                void operator()(const NodeBinExprSub* sub){
                    int j = gen->genExpr(sub->rs);
                    int k = gen->genExpr(sub->ls);
                    i = std::max(j,k);
                    if(dest.has_value())i = std::max((char) i,dest.value().sid);
                    gen->m_output << "   mov " << reg(i,0) << ",0\n";
                    gen->pop(i,j,0);
                    gen->pop(i,k,1);
                    gen-> m_output << "   sub " << reg(i,0) << ", " << reg(i,1) << "\n";
                    gen->push(dest,i);
                    gen->m_instruction += 7;
                }
                void operator()(const NodeBinExprMul* mul){
                    int j = gen->genExpr(mul->rs);
                    int k = gen->genExpr(mul->ls);
                    i = std::max(j,k);
                    if(dest.has_value())i = std::max((char) i,dest.value().sid);
                    gen->m_output << "   mov " << reg(i,0) << ",0\n";
                    gen->pop(i,j,0);
                    gen->pop(i,k,1);
                    
                    gen-> m_output << "   mul " << reg(i,1) << "\n";
                    gen->push(dest,i);
                    gen->m_instruction += 7;
                }
                void operator()(const NodeBinExprDiv* div){
                    int j = gen->genExpr(div->rs);
                    int k = gen->genExpr(div->ls);
                    i = std::max(j,k);
                    if(dest.has_value())i = std::max((char) i,dest.value().sid);
                    gen->m_output << "   mov " << reg(i,0) << ",0\n";
                    gen->pop(i,j,0);
                    gen->pop(i,k,1);
                    
                    gen-> m_output << "   div " << reg(i,1) << "\n";
                    gen->push(dest,i);
                    gen->m_instruction += 7;
                }
                void operator()(const NodeBinExprAdd* add){
                    int j = gen->genExpr(add->rs);
                    int k = gen->genExpr(add->ls);
                    i = std::max(j,k);
                    if(dest.has_value())i = std::max((char) i,dest.value().sid);
                    gen->m_output << "   mov " << reg(i,0) << ",0\n";
                    gen->pop(i,j,0);
                    gen->pop(i,k,1);
                    
                    gen-> m_output << "   add " << reg(i,0) << ", " << reg(i,1) << "\n";
                    gen->push(dest,i);
                    gen->m_instruction += 7;
                }
                void operator()(const NodeBinAreth* areth){
                    int j = gen->genExpr(areth->rs);
                    int k = gen->genExpr(areth->ls);
                    i = std::max(j,k);
                    if(dest.has_value())i = std::max((char) i,dest.value().sid);
                    gen->m_output << "   mov " << reg(i,0) << ",0\n";
                    gen->pop(j,0);
                    gen->pop(k,1);
                    
                    gen->m_lables.back() = gen->m_labelI;
                    gen-> m_output << "   cmp " << reg(i,0) << ", " << reg(k,1) << "\n";
                    i = (int) areth->type;
                    gen->m_instruction += 9;
                }
            };
            BinExprVisitor visitor{.gen = this,.dest = dest};
            std::visit(visitor,binExpr->var);
            return i;
        }

        int genExpr(const NodeExpr* expr,std::optional<PreGen::SizeAble> dest = {}) {
            static int i = -1;
            struct ExprVisitor{
                Generator* gen;
                std::optional<PreGen::SizeAble> dest;
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

        int genTerm(const NodeTerm* term,std::optional<PreGen::SizeAble> dest = {}){
            static int i = -1;
            struct TermVisitor
            {
                Generator* gen;
                std::optional<PreGen::SizeAble> dest;
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
                    auto it = std::find_if(gen->m_vars.back().cbegin(), gen->m_vars.back().cend(), [&](const PreGen::Var& var) {
                        return var.name == id->id.value.value();
                    });
                    if(it == gen->m_vars.back().cend()){
                        std::cerr << "Undeclared identifier: " << id->id.value.value() << "\nat line: " << id->id.line << std::endl;
                        exit(EXIT_FAILURE);
                    }
                    const auto& var = *it;
                    gen->push(var,dest);
                    i = var.sid;
                }
                void operator()(const NodeTermParen* paren) const {
                    i = gen->genExpr(paren->expr,dest);
                }
                void operator()(const FuncCall* call){
                    i = gen->genFuncCall(call);
                    gen->push(dest,i);
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
                    std::optional<PreGen::SizeAble> o = PreGen::RegIns(0,4);
                    gen->genExpr(stmt_exit->expr,o);
                    gen->m_output << "   mov rax, 60\n";
                    gen->m_output << "   syscall\n";
                    gen->m_instruction += 7;
                }
                void operator()(const NodeStmtLet* stmt_let){
                    auto it = std::find_if(gen->m_vars.back().cbegin(), gen->m_vars.back().cend(), [&](const PreGen::Var& var) {
                        return var.name == stmt_let->ident.value.value();
                    });
                    if (it != gen->m_vars.back().cend()){
                        std::cerr << "Identifier already used: " << stmt_let->ident.value.value() << "\nat line: " << stmt_let->ident.line << std::endl;
                        exit(EXIT_FAILURE);
                    }
                    gen->m_varsI++;
                    gen->m_stackPos.back() += regIDSize(stmt_let->sid);
                    PreGen::Var var(stmt_let->sid,gen->m_stackPos.back(),stmt_let->_signed,stmt_let->ident.value.value());
                    gen->m_vars.back().push_back(var);
                    gen->genExpr(stmt_let->expr,var);
                }
                void operator()(const NodeStmtScope* scope){
                    gen->genScope(scope);
                }
                void operator()(const NodeStmtIf* _if) const{
                    gen->gen_if(_if);
                }
                void operator()(const NodeStmtReassign* reassign){
                    auto it = std::find_if(gen->m_vars.back().cbegin(), gen->m_vars.back().cend(), [&](const PreGen::Var& var) {
                        return var.name == reassign->id.value.value();
                    });
                    if(it == gen->m_vars.back().cend()){
                        std::cerr << "Undeclared identifier1: " << reassign->id.value.value() << "\nat line: " << reassign->id.line << std::endl;
                        exit(EXIT_FAILURE);
                    }
                    const auto& var = *it;
                    if(reassign->expr.has_value())gen->genExpr(reassign->expr.value(),PreGen::RegIns(var.sid,1));
                    if(reassign->op == TokenType::eq){
                        gen->move(var,1);
                    }else{
                        switch (reassign->op)
                        {
                        case TokenType::plus_eq:
                            gen->move(0,var);
                            gen->m_output << "   add " << reg(var.sid,0) << "," << reg(var.sid,1) << "\n";
                            gen->move(var,0);
                            break;
                        case TokenType::sub_eq:
                            gen->move(0,var);
                            gen->m_output << "   sub " << reg(var.sid,0) << "," << reg(var.sid,1) << "\n";
                            gen->move(var,0);
                            break;
                        case TokenType::mul_eq:
                            gen->move(0,var);
                            gen->m_output << "   mul " << reg(var.sid,1) << "\n";
                            gen->move(var,0);
                            break;
                        case TokenType::div_eq:
                            gen->move(0,var);
                            gen->m_output << "   div " << reg(var.sid,1) << "\n";
                            gen->move(var,0);
                            break;
                        case TokenType::inc:
                            gen->m_output << "  inc " << sidType(var.sid) << " [rbp - " << var.stackPos << "]\n";
                            break;
                        case TokenType::dec:
                            gen->m_output << "  dec " << sidType(var.sid) << " [rbp - " << var.stackPos << "]\n";
                            break;
                        
                        default:
                            break;
                        }
                    }
                }
                void operator()(const _Return* _return){
                    if(_return->val.has_value()){
                        //TODO Add reference to check if return type is correct
                        int i = gen->m_pre_gen->genExpr(_return->val.value());
                        PreGen::RegIns var(i,0);
                        gen->genExpr(_return->val.value(),var);
                    }
                    gen->m_output << "   pop rbp\n   ret\n";
                }
                void operator()(const FuncCall* call){
                    gen->genFuncCall(call);
                }
            };

            StmtVisitor vis{.gen = this};
            std::visit(vis,stmt->var);
        }

    private:

        void gen_if(const NodeStmtIf* _if){
            int m = m_labelI;
            int j = m_pre_gen->preGen(_if->scope) + 1;
            int k = m_labelI + j;
            for(int i = 0;i < _if->expr.size() - 1;i++){
                auto expr = _if->expr.at(i);
                auto cond = _if->exprCond.at(i);
                m_lables.push_back(-1);
                int l = genExpr(expr);
                if(m_lables.back() == -1){
                    pop(2,0);
                    size_t lable = m_labelI;
                    m_output << "   test rax, rax \n";
                    m_output << "   jnz lable" << lable << "\n";
                    m_lables.back() = lable;
                    m_instruction += 4;
                }else {
                    if(cond == TokenType::_and){
                        m_output << "   " << binjump((TokenType) l) << " lable" << k << "\n";
                    }else if(cond == TokenType::_or){
                        m_output << "   " << binjump((TokenType) l) << " lable" << m_labelI << "\n";
                    }
                }
                m_lables.pop_back();
            }
            auto expr = _if->expr.back();
            m_lables.push_back(-1);
            int l = genExpr(expr);
            if(m_lables.back() == -1){
                pop(2,0);
                size_t lable = m_labelI;
                m_output << "   test rax, rax \n";
                m_output << "   jnz lable" << lable << "\n";
                m_lables.back() = lable;
                m_instruction += 4;
            }else {
                if(_if->exprCond.size() > 0 && _if->exprCond.back() == TokenType::_and){
                    m_output << "   " << binnotjump((TokenType) l) << " lable" << m_labelI << "\n";
                }else{
                    m_output << "   " << binnotjump((TokenType) l) << " lable" << m_labelI << "\n";
                }
            }
            m_lables.pop_back();
                    
            m_output << "   jmp lable" << k << "\n";
            m_output << "lable" << m_labelI << ":\n";
            m_labelI++;
                    
            ScopeStmtVisitor vis{.gen = this};
            std::visit(vis,_if->scope);

            if(_if->scope_else.has_value()){
                m_output << "  jmp lable" << k << "\n";
                m_output << "lable" << m_labelI << ":\n";
                m_labelI++;
                std::visit(vis,_if->scope_else.value());
            }else if(_if->else_if.has_value()){
                m_output << "  jmp lable" << k << "\n";
                m_output << "lable" << m_labelI << ":\n";
                m_labelI++;
                gen_if(_if->else_if.value());
            }
            m_output << "lable" << m_labelI << ":\n";
            m_labelI++;
        }

        void genfunc(Function* func){
            m_vars.push_back({});
            m_stackPos.push_back(0);
            for(int i = 0;i < func->paramName.size();i++){
                m_stackPos.back() += regIDSize((int) func->paramType.at(i));
                PreGen::Var var((int) func->paramType.at(i),m_stackPos.back(),false,func->paramName.at(i));
                m_vars.back().push_back(var);
            }
            m_output << func->fullName << ":\n  push rbp\n  mov rbp,rsp\n";
            genScope(func->scope);
            m_output << "   pop rbp\n   ret\n";
            m_stackPos.pop_back();
            m_vars.pop_back();
        }

        char genFuncCall(const FuncCall* call){
            std::string pFunc = call->name;
            std::vector<int> sids {};
            
            for(auto expr : call->exprs){
                sids.push_back(m_pre_gen->genExpr(expr,m_vars.back()));
            }
            std::string name = getName(call->name,sids);
            if(!m_funcs.contains(name)){
                std::cerr << "Unkown function: " << name << std::endl;
                exit(EXIT_FAILURE);
            }
            Function* func = m_funcs.at(name);
            callMod();
            size_t buf_stackPos = m_stackPos.back();
            for(int i = 0;i < call->exprs.size();i++){
                m_stackPos.back() += regIDSize((int) func->paramType.at(i));
                PreGen::Var var((int) sids.at(i),m_stackPos.back(),false,func->paramName.at(i));
                genExpr(call->exprs.at(i),var);
            }
            m_stackPos.back() = buf_stackPos;
            m_output << "   call " << charType(func->_return) << name << "\n";
            callModBack();
            return (int) func->_return;
        }

        void callMod(){
            m_output << "   sub rsp," << m_stackPos.back() - m_rsp << "\n";
            m_stackPos.back() += 16;
            m_rsp = m_stackPos.back();
        }

        void callModBack(){
            m_stackPos.back() -= 16;
            m_rsp = m_stackPos.back();
        }

        std::string getName(std::string name,std::vector<int> types){
            std::stringstream ss;
            ss << name;
            for(auto type : types){
                ss << charType((TokenType) type);
            }
            return ss.str();
        }

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
            Scope scope{.stack = m_stackPos.back(),.var = m_varsI};
            m_scopes.push_back(scope);
        }

        void endScope() {
            Scope scope = m_scopes.back();
            m_stackPos.back() = scope.stack;
            size_t i = m_varsI - scope.var;
            for(int j = 0;j < i;j++){
                m_vars.back().pop_back();
            }
            m_varsI -= i;
            m_scopes.pop_back();
        }

        std::string createLabel(){
            std::stringstream ss;
            ss << "lable" << m_labelI++;
            return ss.str();
        }

        void writeRam(PreGen::SizeAble var,long val){
            m_output << "   mov " << regIDSize(var.sid) << " [rbp - " << m_stackPos.back() - var.stackPos  - regIDSize(var.sid) << "]," << val << "\n";
            m_instruction += 4;
        }

        void readRam(PreGen::SizeAble var,int regi){
            m_output << "   mov " << reg(var.sid,regi) << "," << regIDSize(var.sid) << " [rbp - " << /*m_stackPos.back() -*/ var.stackPos  - regIDSize(var.sid) << "]\n";
            m_instruction += 4;
        }

        void push(PreGen::Var var,std::optional<PreGen::SizeAble> dest){
            
            if(dest.has_value()){
                if(dest.value().mem == false){
                    if(dest.value().sid > var.sid){
                        m_output << "   movsx " << reg(dest.value().sid,dest.value().stackPos) << ", " << sidType(var.sid) << " [rbp - " << var.stackPos << "]\n";
                        m_instruction += 5;
                    }else{
                        m_output << "   mov " << reg(var.sid,dest.value().stackPos) << ", " << sidType(var.sid) << " [rbp - " << var.stackPos << "]\n";
                        m_instruction += 4;
                    }
                }else{
                    copFVar(var,dest.value());
                }
            }else copFVar(var);
        }

        void move(PreGen::SizeAble var,char regId){
            if(!var.mem){
                std::cerr << "Expected Variable but found Register Inserter" << std::endl;
                exit(EXIT_FAILURE);
            }
            m_output << "   mov " << sidType(var.sid % 4) << " [rbp - " << var.stackPos << "]," << reg(var.sid % 4,regId) << "\n";
            m_instruction += 4;
        }

        void move(char regId,PreGen::SizeAble var){
            if(!var.mem){
                std::cerr << "Expected Variable but found Register Inserter" << std::endl;
                exit(EXIT_FAILURE);
            }
            m_output << "   mov " << reg(var.sid % 4,regId) << "," << sidType(var.sid) << " [rbp - " << var.stackPos << "]\n";
            m_instruction += 4;
        }

        void push(std::optional<PreGen::SizeAble> dest,char i){
            if(dest.has_value()){
                        if(dest.value().mem){
                            m_output << "   mov " << sidType(dest.value().sid % 4) << " [rbp - " << dest.value().stackPos << "]," << reg(dest.value().sid % 4,0) << "\n";
                            m_instruction += 4;
                        }else{
                            m_output << "   mov " << reg(dest.value().sid % 4,dest.value().stackPos) << "," << reg(dest.value().sid % 4,0) << "\n";
                            m_instruction += 2;
                        }
            }else push(i,0);
        }

        void push(int sid,int regi){
            m_stackPos.back() += regIDSize(sid % 4);
            m_output << "   mov "<< sidType(sid % 4) <<" [rbp - " << m_stackPos.back() << "]," << reg(sid % 4,regi) << "\n";
            m_varsI++;
            
            m_instruction += 4;
        }
        
        void copFVar(PreGen::Var var){
            m_stackPos.back() += regIDSize(var.sid);
            m_output << "   mov " << reg(var.sid % 4,0) << ","<< sidType(var.sid % 4) <<" [rbp - " << var.stackPos << "]\n";
            m_output << "   mov "<< sidType(var.sid % 4) <<" [rbp - " << m_stackPos.back() << "]," << reg(var.sid % 4,0) << "\n";
            m_varsI++;
            m_instruction += 8;
        }

        void copFVar(PreGen::SizeAble dest,PreGen::SizeAble src){
            if(!dest.mem || !src.mem){
                std::cerr << "Expected Variable but found Register Inserter" << std::endl;
                exit(EXIT_FAILURE);
            }
            m_output << "   mov " << reg(src.sid % 4,0) << ","<< sidType(src.sid % 4) <<" [rbp - " << src.stackPos << "]\n";
            m_output << "   mov "<< sidType(src.sid % 4) <<" [rbp - " << dest.stackPos << "]," << reg(src.sid % 4,0) << "\n";
            m_varsI++;
            m_instruction += 8;
        }

        void pop(int sid,int regi){
            m_varsI--;
            m_output << "   mov " << reg(sid % 4,regi) << ","<< sidType(sid % 4) <<" [rbp - " << m_stackPos.back() << "]\n";
            m_stackPos.back() -= regIDSize(sid);
            m_instruction += 4;
        }

        void pop(int regsid,int sid,int regi){
            m_varsI--;
            if(regsid > sid)m_output << "   movsx " << reg(regsid,regi) << ","<< sidType(sid) <<" [rbp - " << m_stackPos.back() << "]\n";
            else m_output << "   mov " << reg(regsid,regi) << ","<< sidType(sid) <<" [rbp - " << m_stackPos.back() << "]\n";
            m_stackPos.back() -= regIDSize(sid);
            m_instruction += 4;
        }

        struct Scope{
            size_t stack;
            size_t var;
        };

        const NodeProgram m_prog;
        std::stringstream m_output;
        std::vector<size_t> m_stackPos {};
        size_t m_rsp = 0;
        size_t m_varsI = 0;
        std::vector<std::vector<PreGen::Var>> m_vars {};
        std::vector<Scope> m_scopes;
        std::vector<int> m_lables;
        std::unordered_map<std::string,Function*> m_funcs = {};
        size_t m_labelI = 0;
        size_t m_stmtI;
        size_t m_instruction = 0x401000;
        PreGen* m_pre_gen;
};