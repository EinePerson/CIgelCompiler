#pragma once

#include "preGenerator.hpp"

#include <sstream>
#include <map>
#include <cmath>
#include <unordered_map>
#include <filesystem>

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
        std::cerr << "Size has to be at least 1 and not bigger then 8, but is: " << size << std::endl;
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

        void gen_prog(SrcFile* file){
            m_output.str(std::string());
            m_fileOut.str(std::string());
            m_file = file;
            if(m_file->isMain) m_output << "section .text\n     global _start\n_start:\npush rbp\nmov rbp,rsp\n";

            m_stmtI = 0;
            m_vars.emplace_back();
            m_stackPos.push_back(0);
            m_stmts = file->stmts;
            while (m_stmtI < m_stmts.size()){
                const NodeStmt* stmt = m_stmts.at(m_stmtI);
                gen_stmt(stmt);
                m_stmtI++;
            }
            m_vars.pop_back();
            m_stackPos.pop_back();

            if(m_file->isMain) {
                m_output << "pop rbp\n";
                m_output << "   mov rax, 60\n";
                m_output << "   mov rdi, 0\n";
                m_output << "   syscall\n";
            }

            for(const auto& pair : file->funcs){
                genfunc(pair.second);
            }

            for(auto include:file->includes){
                m_fileOut << "%include " << '"' << "./build/cmp/" << include->fullName << ".asm" << '"' << "\n";
            }
            for(const auto& ext:m_externFunc){
                m_fileOut << "extern " << ext << "\n";
            }

            m_fileOut << m_output.str();

            write(m_fileOut.str(),"./build/cmp/" + file->fullName + ".asm");
            m_output.clear();
            m_fileOut.clear();
            m_externFunc.clear();
        }

        static void write(const std::string& str,const std::string& name){
            std::fstream file(name,std::ios::out);
            file << str;
            file.close();
        }

        int genBinExpr(const NodeBinExpr* binExpr,std::optional<PreGen::PreGen::SizeAble> dest = {}){
            static uint i = -1;
            struct BinExprVisitor{
                Generator* gen;
                std::optional<PreGen::SizeAble> dest;
                void operator()(const NodeBinExprSub* sub){
                    int j = gen->genExpr(sub->rs);
                    int k = gen->genExpr(sub->ls);
                    i = std::max(j,k);
                    if(dest.has_value())i = std::max(i,dest.value().sid);
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
                    if(dest.has_value())i = std::max(i,dest.value().sid);
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
                    if(dest.has_value())i = std::max(i,dest.value().sid);
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
                    if(dest.has_value())i = std::max(i,dest.value().sid);
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
                    if(dest.has_value())i = std::max(i,dest.value().sid);
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
                void operator()(const FuncCall* call) const{
                    i = gen->genFuncCall(call);
                    gen->push(dest,i);
                }

                void operator()(const NodeTermArrayAcces* arrAcc) const{
                    auto it = std::find_if(gen->m_vars.back().cbegin(), gen->m_vars.back().cend(), [&](const PreGen::Var& var) {
                        return var.name == arrAcc->id.value.value();
                    });
                    if(it == gen->m_vars.back().cend()){
                        std::cerr << "Undeclared identifier: " << arrAcc->id.value.value() << "\nat line: " << arrAcc->id.line << std::endl;
                        exit(EXIT_FAILURE);
                    }
                    const auto& var = *it;
                    if(var.primitive){
                        std::cerr << "Cannot acces pirimitive variables \nat " << arrAcc->id.file << ":" << arrAcc->id.line << std::endl;
                        exit(EXIT_FAILURE);
                    }
                    if(var.size != arrAcc->exprs.size()){
                        std::cerr << "Expected " << var.size << " Elements but found " << arrAcc->exprs.size() << "\nat:" << arrAcc->id.file << ":" << arrAcc->id.line << std::endl;
                        exit(EXIT_FAILURE);
                    }
                    gen->arrAdr(var,arrAcc->exprs);
                    gen->m_output << "  mov " << reg(var.sid,1) << ", " << sidType(var.sid) << "[rcx]\n";
                    //gen->push(dest,ss.str(),var.sid);
                    gen->moveInto(dest.value(),1);
                }
            };
            TermVisitor visitor({.gen = this,.dest = dest});
            std::visit(visitor,term->var);
            if(term->outer.has_value())i = genTerm(term->outer.value());
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
                    gen->genLet(stmt_let);

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
                        gen->areth(var,reassign->op);
                    }
                }
                void operator()(const NodeStmtArrReassign* reassign){
                    auto it = std::find_if(gen->m_vars.back().cbegin(), gen->m_vars.back().cend(), [&](const PreGen::Var& var) {
                        return var.name == reassign->id.value.value();
                    });
                    if(it == gen->m_vars.back().cend()){
                        std::cerr << "Undeclared identifier: " << reassign->id.value.value() << "\nat line: " << reassign->id.line << std::endl;
                        exit(EXIT_FAILURE);
                    }
                    const auto& var = *it;
                    if(var.primitive){
                        std::cerr << "Cannot acces pirimitive variables \nat " << reassign->id.file << ":" << reassign->id.line << std::endl;
                        exit(EXIT_FAILURE);
                    }
                    if(var.size != reassign->exprs.size()){
                        std::cerr << "Expected " << var.size << " Elements but found " << reassign->exprs.size() << "\nat:" << reassign->id.file << ":" << reassign->id.line << std::endl;
                        exit(EXIT_FAILURE);
                    }
                    gen->arrAdr(var,reassign->exprs);
                    if(reassign->expr.has_value())gen->genExpr(reassign->expr.value(),PreGen::RegIns(var.sid,1));
                    if(reassign->op == TokenType::eq){
                        gen->m_output << "   mov " << sidType(var.sid % 4) << " [rcx]," << reg(var.sid % 4,1) << "\n";
                    }else{
                        gen->areth(var,reassign->op, 2);
                    }
                    //gen->m_output << "mov rax,0\n";
                    //gen->m_output << "mov DWORD [rax],5\n";
                }
                void operator()(const Return* _return){
                    if(_return->val.has_value()){
                        //TODO Add reference to check if return type is correct
                        int i = gen->genExpr(_return->val.value());
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

        /**
         * Flattens the array and returns the addres in rcx
         * Throwing errors is not possible so the amount of Expressions HAS to match the dimensions of the array
         * @param var The array in question
         * @param eprs the expressions for every dimension
         */
        void arrAdr(PreGen::Var var,std::vector<NodeExpr*> exprs){
            /*if(var.size != exprs.size()){
                std::cerr << "Expected " << var.size << " Elements but found " << exprs.size() << "\nat:" << var. << ":" << id.line << std::endl;
                exit(EXIT_FAILURE);
            }*/
            //gen->m_output << "mov rcx,0\n";
            m_output << "mov rdx,0\n";
            m_output << "mov edx,DWORD [rbp - " << var.stackPos + 4 * (var.size - 1) << "]\n";
            //gen->m_output << "mov rdx," << "DWORD [rbp - " << var.stackPos + (arrAcc->exprs.size() * 4) << "]\n";
            genExpr(exprs.back(),PreGen::RegIns(3,2));
            for(size_t j = exprs.size() - 2;j < exprs.size();j--){
                genExpr(exprs[j],PreGen::RegIns(3,0));
                m_output << "movsx rbx,DWORD [rbp - " << var.stackPos + 4 * j << "]\n";
                m_output << "imul rax,rdx\n";
                m_output << "add rcx,rax\n";
                m_output << "imul rdx,rbx\n";
                //gen->m_output << "add rcx,rax\n";
                //gen->m_output << "imul rdx,rbx\n";
            }
            m_output << "  lea rax,[rbp - " << var.stackPos << "]\n";
            m_output << "  imul rcx," << regIDSize(var.size) << "\n";
            m_output << "  add rcx,rax\n";
            m_output << "  add rcx," << var.size * 4 << "\n";
        }

        void genLet(const NodeStmtLet* let){
            struct LetGen{
                Generator* gen;
                Token ident;

                void operator()(NodeStmtPirimitiv* pirim){
                    auto it = std::find_if(gen->m_vars.back().cbegin(), gen->m_vars.back().cend(), [&](const PreGen::Var& var) {
                        return var.name == ident.value.value();
                    });
                    if (it != gen->m_vars.back().cend()){
                        std::cerr << "Identifier already used: " << ident.value.value() << "\nat line: " << ident.line << std::endl;
                        exit(EXIT_FAILURE);
                    }
                    gen->m_varsI++;
                    gen->m_stackPos.back() += regIDSize(pirim->sid);
                    PreGen::Var var(pirim->sid,gen->m_stackPos.back(),pirim->_signed,ident.value.value(), false);
                    gen->m_vars.back().push_back(var);
                    gen->genExpr(pirim->expr,var);
                }

                void operator()(NodeStmtArr* newStmt) const{
                    auto it = std::find_if(gen->m_vars.back().cbegin(), gen->m_vars.back().cend(), [&](const PreGen::Var& var) {
                        return var.name == ident.value.value();
                    });
                    if (it != gen->m_vars.back().cend()){
                        std::cerr << "Identifier already used: " << ident.value.value() << "\nat line: " << ident.line << std::endl;
                        exit(EXIT_FAILURE);
                    }
                    size_t pos = gen->m_stackPos.back() + 4;
                    size_t size = -1;
                    if(newStmt->fixed){
                        size = 1;
                        for (const auto &item: newStmt->size) {
                            if (item <= 0) {
                                std::cerr << "Size of Array has to be bigger 0" << std::endl;
                                exit(EXIT_FAILURE);
                            }
                            gen->pushLit(2,item);
                            size *= item;
                        }
                        //gen->m_stackPos.back() += size;
                    } else{
                        //TODO add dynamic sized Arrays
                    }
                    gen->m_varsI++;
                    PreGen::Var var(newStmt->sid,pos, newStmt->_signed,ident.value.value(),newStmt->size.size());
                    gen->m_stackPos.back() += size;
                    gen->m_vars.back().push_back(var);
                }
            };

            LetGen gen{.gen = this,.ident = let->ident};
            std::visit(gen,let->type);
        }

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
                PreGen::Var var((int) func->paramType.at(i),m_stackPos.back(),false,func->paramName.at(i), false);
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
            Function* func;
            if(auto fun = m_file->findFunc(name)){
                func = fun.value().func;
                if(fun.value().state == FunctionState::_use)m_externFunc.push_back(fun.value().func->fullName);
            }else{
                std::cerr << "Unkown function: " << name << std::endl;
                exit(EXIT_FAILURE);
            }
            callMod();
            size_t buf_stackPos = m_stackPos.back();
            for(int i = 0;i < call->exprs.size();i++){
                m_stackPos.back() += regIDSize((int) func->paramType.at(i));
                PreGen::Var var((int) sids.at(i),m_stackPos.back(),false,func->paramName.at(i), false);
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

        void areth(PreGen::SizeAble var,TokenType op,char regP = -1){
            switch (op)
            {
                case TokenType::plus_eq:
                    if(regP == -1)move(0,var);
                    else m_output << "   mov " << reg(var.sid % 4,0) << "," << sidType(var.sid) << " [" << reg(3,regP) << "]\n";
                    m_output << "   add " << reg(var.sid,0) << "," << reg(var.sid,1) << "\n";
                    if(regP == -1)move(var,0);
                    else m_output << "   mov " << sidType(var.sid % 4) << " [" << reg(3,regP) << "]," << reg(var.sid % 4,0) << "\n";
                    break;
                case TokenType::sub_eq:
                    if(regP == -1)move(0,var);
                    else m_output << "   mov " << reg(var.sid % 4,0) << "," << sidType(var.sid) << " [" << reg(3,regP) << "]\n";
                    m_output << "   sub " << reg(var.sid,0) << "," << reg(var.sid,1) << "\n";
                    if(regP == -1)move(var,0);
                    else m_output << "   mov " << sidType(var.sid % 4) << " [" << reg(3,regP) << "]," << reg(var.sid % 4,0) << "\n";
                    break;
                case TokenType::mul_eq:
                    if(regP == -1)move(0,var);
                    else m_output << "   mov " << reg(var.sid % 4,0) << "," << sidType(var.sid) << " [" << reg(3,regP) << "]\n";
                    m_output << "   mul " << reg(var.sid,1) << "\n";
                    if(regP == -1)move(var,0);
                    else m_output << "   mov " << sidType(var.sid % 4) << " [" << reg(3,regP) << "]," << reg(var.sid % 4,0) << "\n";
                    break;
                case TokenType::div_eq:
                    if(regP == -1)move(0,var);
                    else m_output << "   mov " << reg(var.sid % 4,0) << "," << sidType(var.sid) << " [" << reg(3,regP) << "]\n";
                    m_output << "   div " << reg(var.sid,1) << "\n";
                    if(regP == -1)move(var,0);
                    else m_output << "   mov " << sidType(var.sid % 4) << " [" << reg(3,regP) << "]," << reg(var.sid % 4,0) << "\n";
                    break;
                case TokenType::inc:
                    m_output << "  inc " << sidType(var.sid) << " [rbp - " << var.stackPos << "]\n";
                    break;
                case TokenType::dec:
                    m_output << "  dec " << sidType(var.sid) << " [rbp - " << var.stackPos << "]\n";
                    break;

                default:
                    break;
            }
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

        void move(PreGen::SizeAble var,u_char regId){
            if(!var.mem){
                std::cerr << "Expected Variable but found Register Inserter" << std::endl;
                exit(EXIT_FAILURE);
            }
            m_output << "   mov " << sidType(var.sid % 4) << " [rbp - " << var.stackPos << "]," << reg(var.sid % 4,regId) << "\n";
            m_instruction += 4;
        }

        void move(u_char regId,PreGen::SizeAble var){
            if(!var.mem){
                std::cerr << "Expected Variable but found Register Inserter" << std::endl;
                exit(EXIT_FAILURE);
            }
            m_output << "   mov " << reg(var.sid % 4,regId) << "," << sidType(var.sid) << " [rbp - " << var.stackPos << "]\n";
            m_instruction += 4;
        }

        void moveInto(PreGen::SizeAble var,u_char srcId){
            if(var.mem){
                m_output << "   mov " << sidType(var.sid) << "[rbp - " << var.stackPos << "]," << reg(var.sid,srcId) << "\n";
            }else{
                m_output << "   mov " << reg(var.sid,var.stackPos) << "," << reg(var.sid,srcId) << "\n";
            }
        }

        void push(std::optional<PreGen::SizeAble> dest,u_char i){
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

        void push(uint sid,int regi){
            m_stackPos.back() += regIDSize(sid % 4);
            m_output << "   mov "<< sidType(sid % 4) <<" [rbp - " << m_stackPos.back() << "]," << reg(sid % 4,regi) << "\n";
            m_varsI++;
            
            m_instruction += 4;
        }

        void pushLit(uint sid,long val){
            m_stackPos.back() += regIDSize(sid % 4);
            m_output << "   mov "<< sidType(sid % 4) <<" [rbp - " << m_stackPos.back() << "]," << val << "\n";

            m_instruction += 4;
        }
        
        void copFVar(PreGen::Var var){
            m_stackPos.back() += regIDSize(var.sid);
            m_output << "   mov " << reg(var.sid % 4,0) << ","<< sidType(var.sid % 4) <<" [rbp - " << var.stackPos << "]\n";
            m_output << "   mov "<< sidType(var.sid % 4) <<" [rbp - " << m_stackPos.back() << "]," << reg(var.sid % 4,0) << "\n";
            m_varsI++;
            m_instruction += 8;
        }

        void push(std::optional<PreGen::SizeAble> dest,std::string expr,u_char sid){
            if(dest.has_value()){
                if(dest.value().mem){
                    m_output << "   mov " << sidType(dest.value().sid % 4) << " [rbp - " << dest.value().stackPos << "],[rbp - " << expr << "]\n";
                    m_instruction += 4;
                }else{
                    m_output << "   mov " << reg(dest.value().sid % 4,dest.value().stackPos) << ",[rbp - " << expr << "]\n";
                    m_instruction += 2;
                }
            }else copFVar(sid,expr);
        }

        void copFVar(char sid,std::string expr){
            m_stackPos.back() += regIDSize(sid);
            m_output << "   mov " << reg(sid % 4,0) << ","<< sidType(sid % 4) <<" [rbp - " << expr << "]\n";
            m_output << "   mov "<< sidType(sid % 4) <<" [rbp - " << m_stackPos.back() << "]," << reg(sid % 4,0) << "\n";
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

        void pop(uint sid,int regi){
            m_varsI--;
            m_output << "   mov " << reg(sid % 4,regi) << ","<< sidType(sid % 4) <<" [rbp - " << m_stackPos.back() << "]\n";
            m_stackPos.back() -= regIDSize(sid);
            m_instruction += 4;
        }

        void pop(uint regsid,int sid,int regi){
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

        SrcFile* m_file;
        std::stringstream m_output;
        std::stringstream m_fileOut;
        std::vector<size_t> m_stackPos {};
        size_t m_rsp = 0;
        size_t m_varsI = 0;
        std::vector<std::vector<PreGen::Var>> m_vars {};
        std::vector<std::string> m_externFunc;
        std::vector<Scope> m_scopes;
        std::vector<int> m_lables;
        std::vector<NodeStmt*> m_stmts;
        size_t m_labelI = 0;
        size_t m_stmtI;
        size_t m_instruction = 0x401000;
        PreGen* m_pre_gen;
};