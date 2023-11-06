#pragma once 

#include <vector>
#include <variant>
#include <iostream>

//#include "../tokenizer.h"
//#include "../util/arena.hpp"
//#include "../CompilerInfo/CompilerInfo.hpp"

char charType(TokenType type){
    switch (type)
    {
    case TokenType::_byte:
        return 'b';
    case TokenType::_short:
        return 's';
    case TokenType::_int:
        return 'i';
    case TokenType::_long:
        return 'l';
    case TokenType::_ubyte:
        return 'B';
    case TokenType::_ushort:
        return 'S';
    case TokenType::_uint:
        return 'I';
    case TokenType::_ulong:
        return 'L';
    case TokenType::_void:
        return 'V';

    default:
        std::cerr << "Expected Data Type but found" << (int) type << std::endl;
        exit(EXIT_FAILURE);
    }
}

class Parser{
    public:
        inline explicit Parser() : m_alloc(1024 * 1024 * 4){}

        std::optional<NodeTerm*> parseTerm(){
            if(auto int_lit = tryConsume(TokenType::int_lit)){
                auto nodeInt = m_alloc.alloc<NodeTermIntLit>();
                nodeInt->int_lit = int_lit.value();
                auto expr = m_alloc.alloc<NodeTerm>();
                expr->var = nodeInt;
                return expr;
            }
            else if(auto id = tryConsume(TokenType::id)){
                if(tryConsume(TokenType::openParenth)){
                    auto call = m_alloc.alloc<FuncCall>();
                    call->name = id.value().value.value();
                    char i = 0;
                    while(auto expr = parseExpr()){
                        if(i)tryConsume(TokenType::comma,"Expected ','");
                        call->exprs.push_back(expr.value());
                        if(!tryConsume(TokenType::comma))i = 1;
                    }
                    tryConsume(TokenType::closeParenth,"Expected ')'");
                    auto expr = m_alloc.alloc<NodeTerm>();
                    expr->var = call;
                    return expr;
                }else{
                    auto* nodeId = m_alloc.alloc<NodeTermId>();
                    nodeId->id = id.value();
                    auto expr = m_alloc.alloc<NodeTerm>();
                    expr->var = nodeId;
                    return expr;
                }
            }else if(auto paren = tryConsume(TokenType::openParenth)){
                auto expr = parseExpr();
                if(!expr.has_value()){
                    std::cerr << "Expected Expression" << "\nat line: " << paren.value().line << std::endl;
                    exit(EXIT_FAILURE);
                }
                tryConsume(TokenType::closeParenth,"Expected ')'");
                auto term_paren = m_alloc.alloc<NodeTermParen>();
                term_paren->expr = expr.value();
                auto term = m_alloc.alloc<NodeTerm>();
                term->var = term_paren;
                return term;
            }else return {};
        }

        std::optional<NodeExpr*> parseExpr(int minPrec = 0){
            std::optional<NodeTerm*> termLs = parseTerm();
            if(!termLs.has_value())return {};
            auto exprLs = m_alloc.alloc<NodeExpr>();
            exprLs->var = termLs.value();

            while(true){
                std::optional<Token> token = peak();
                std::optional<int> precI;
                if(token.has_value()){
                    precI = prec(token->type);
                    if(!precI.has_value() || precI < minPrec)break;
                }else break;
                Token op = consume();
                int nMinPrec = precI.value() + 1;
                auto exprR = parseExpr(nMinPrec);
                if(!exprR.has_value()){
                    std::cerr << "Unable to parse" << std::endl;
                    exit(EXIT_FAILURE);
                }
                auto expr = m_alloc.alloc<NodeBinExpr>();
                auto exprLs2 = m_alloc.alloc<NodeExpr>();
                if(op.type == TokenType::plus){
                    auto add = m_alloc.alloc<NodeBinExprAdd>();
                    exprLs2->var = exprLs->var;
                    add->ls = exprLs2;
                    add->rs = exprR.value();
                    expr->var = add;
                }else if(op.type == TokenType::mul){
                    auto mul = m_alloc.alloc<NodeBinExprMul>();
                    exprLs2->var = exprLs->var;
                    mul->ls = exprLs2;
                    mul->rs = exprR.value();
                    expr->var = mul;
                }else if(op.type == TokenType::div){
                    auto div = m_alloc.alloc<NodeBinExprDiv>();
                    exprLs2->var = exprLs->var;
                    div->ls = exprLs2;
                    div->rs = exprR.value();
                    expr->var = div;
                }else if(op.type == TokenType::sub){
                    auto sub = m_alloc.alloc<NodeBinExprSub>();
                    exprLs2->var = exprLs->var;
                    sub->ls = exprLs2;
                    sub->rs = exprR.value();
                    expr->var = sub;
                }else if(op.type >= TokenType::equal && op.type <= TokenType::small){
                    auto areth = m_alloc.alloc<NodeBinAreth>();
                    exprLs2->var = exprLs->var;
                    areth->ls = exprLs2;
                    areth->rs = exprR.value();
                    areth->type = op.type;
                    expr->var = areth;
                }else{
                    std::cerr << "Unkown operator" << std::endl;
                    exit(EXIT_FAILURE);
                }
                exprLs->var = expr;
            
            }
            return exprLs;
        }

        std::optional<NodeStmtScope*> parseScope(){
            if(!tryConsume(TokenType::openCurl).has_value())return {};
            auto scope = m_alloc.alloc<NodeStmtScope>();
            while(auto stmt = parseStmt()){
                scope->stmts.push_back(stmt.value());
            }
            tryConsume(TokenType::closeCurl,"Expected '}'");
            return scope;
        }

        std::optional<NodeStmt*> parseStmt(){
            if(peak().value().type == TokenType::_exit && peak(1).has_value() && peak(1).value().type == TokenType::openParenth){
                consume();
                consume();
                NodeStmtExit* stmt = m_alloc.alloc<NodeStmtExit>();
                if(auto nodeExpr = parseExpr()){
                    stmt->expr = nodeExpr.value();
                }else{
                    std::cerr << "Invalid Expression1" << std::endl;
                    exit(EXIT_FAILURE);
                }
                tryConsume(TokenType::closeParenth, "Expected `)`");
                tryConsume(TokenType::semi,"Expected ';'");
                auto stmtR = m_alloc.alloc<NodeStmt>();
                stmtR->var = stmt;
                return stmtR;
            }else if(peak().has_value() && peak().value().type <= TokenType::_ulong && peak(1).has_value() && peak(1).value().type == TokenType::id && peak(2).has_value() && peak(2).value().type == TokenType::eq){
                auto stmt = m_alloc.alloc<NodeStmtLet>();
                stmt->sid = (char) peak().value().type;
                stmt->_signed = consume().type <= TokenType::_long;
                stmt->ident = consume();
                consume();
                if(auto expr = parseExpr())stmt->expr = expr.value();
                else {
                    std::cerr << "Unexpected expression1" << std::endl;
                    exit(EXIT_FAILURE);
                }
                tryConsume(TokenType::semi,"Expected ';'");
                auto stmtR = m_alloc.alloc<NodeStmt>();
                stmtR->var = stmt;
                return stmtR;
            }else if(auto scope = parseScope()){
                auto stmt = m_alloc.alloc<NodeStmt>();
                stmt->var = scope.value();
                return stmt;
            }else if(auto _if = tryConsume(TokenType::_if)){
                auto stmt = m_alloc.alloc<NodeStmt>();
                stmt->var = parseIf();
                return stmt;
            } else if(auto id = tryConsume(TokenType::id)){
                if(tryConsume(TokenType::openParenth)){
                    auto call = m_alloc.alloc<FuncCall>();
                    call->name = id.value().value.value();
                    char i = 0;
                    while(auto expr = parseExpr()){
                        if(i)tryConsume(TokenType::comma,"Expected ','");
                        call->exprs.push_back(expr.value());
                        if(!tryConsume(TokenType::comma))i = 1;
                    }
                    tryConsume(TokenType::closeParenth,"Expected ')'");
                    tryConsume(TokenType::semi,"Expected ';'");
                    auto stmt = m_alloc.alloc<NodeStmt>();
                    stmt->var = call;
                    return stmt;
                }else{
                    auto reassign = m_alloc.alloc<NodeStmtReassign>();
                    reassign->id = id.value();
                    auto stmt = m_alloc.alloc<NodeStmt>();
                    if(peak().has_value() && peak().value().type >= TokenType::eq && peak().value().type <= TokenType::pow_eq){
                        reassign->op = consume().type;
                
                        if(auto expr = parseExpr())reassign->expr = expr.value();
                        else {
                            std::cerr << "Unexpected expression2" << std::endl;
                            exit(EXIT_FAILURE);
                        }
                    }
                    else if(peak().has_value() && peak().value().type == TokenType::inc || peak().value().type == TokenType::dec){
                    reassign->op = consume().type;
                    }else {
                        std::cerr << "Expected Assignment Operator" << std::endl;
                        exit(EXIT_FAILURE);
                    }
                    stmt->var = reassign;
                    tryConsume(TokenType::semi,"Expected ';'");
                    return stmt;
                }
            }else if(peak().has_value() && (peak().value().type == TokenType::inc || peak().value().type == TokenType::dec)){
                auto reassign = m_alloc.alloc<NodeStmtReassign>();
                reassign->op = peak().value().type;
                consume();
                if(auto id = tryConsume(TokenType::id)){
                    reassign->id = id.value();
                }else {
                    std::cerr << "Expected Identifier" << std::endl;
                    exit(EXIT_FAILURE);
                }
                
                tryConsume(TokenType::semi,"Expected ';'");
                auto stmt = m_alloc.alloc<NodeStmt>();
                stmt->var = reassign;
                return stmt;
            }else if(peak().has_value() && peak().value().type == TokenType::_return){
                consume();
                auto _return = m_alloc.alloc<Return>();
                if(auto expr = parseExpr()){
                    _return->val = expr.value();
                }
                auto stmt = m_alloc.alloc<NodeStmt>();
                stmt->var = _return;
                tryConsume(TokenType::semi,"Expected ';'");
                return stmt;
            }{
                return {};
            }
        }

        NodeStmtIf* parseIf(){
            tryConsume(TokenType::openParenth,"Expected '('");
            auto stmt_if = m_alloc.alloc<NodeStmtIf>();
            int i = 0;

            while(auto expr = parseExpr()){
                i++;
                stmt_if->expr.push_back(expr.value());
                if(auto cond = tryConsume(TokenType::_and))stmt_if->exprCond.push_back(cond.value().type);
                else if(auto cond = tryConsume(TokenType::_or))stmt_if->exprCond.push_back(cond.value().type);
                else if(auto cond = tryConsume(TokenType::_xor))stmt_if->exprCond.push_back(cond.value().type);
            }
            if(!i){
                std::cerr << "Invalid Expression2" << std::endl;
                exit(EXIT_FAILURE);
            }
            tryConsume(TokenType::closeParenth,"Expected ')'2");
            if(auto scope = parseScope()){
                stmt_if->scope = scope.value();
            }else if(auto stms = parseStmt()){
                stmt_if->scope = stms.value();
            }else {
                std::cerr << "Invalid Scope" << std::endl;
                exit(EXIT_FAILURE);
            }
                
            if(tryConsume(TokenType::_else)){
                if(tryConsume(TokenType::_if)){
                    stmt_if->else_if = parseIf();
                }else if(auto scope = parseScope()){
                    stmt_if->scope_else = scope.value();
                }else if(auto stms = parseStmt()){
                    stmt_if->scope_else = stms.value();
                }else {
                    std::cerr << "Invalid else Scope" << std::endl;
                    exit(EXIT_FAILURE);
                }
            }
            return stmt_if;
        }

        std::optional<Function*> parseFunc(){
            auto func = m_alloc.alloc<Function>();
            if(peak().has_value() && (peak().value().type <= TokenType::_ulong || peak().value().type == TokenType::_void))func->_return = consume().type;
            else return {};

            auto hname = tryConsume(TokenType::id,"Expected identifier").value;

            

            tryConsume(TokenType::openParenth,"Expected '('");
            char i = 0;
            while (peak().has_value() && peak().value().type <= TokenType::_ulong)
            {
                if(i)tryConsume(TokenType::comma,"Expected ','");
                func->paramType.push_back(consume().type);
                func->paramName.push_back(tryConsume(TokenType::id,"Expected identifier").value.value());
                if(!tryConsume(TokenType::comma))i = 1;
            }

            tryConsume(TokenType::closeParenth,"Expected ')'");
            if(auto scope = parseScope())func->scope = scope.value();
            else {
                std::cerr << "Expected Scope" << std::endl;
                exit(EXIT_FAILURE);
            }

            std::stringstream ss;
            ss << hname.value();
            for(auto type : func->paramType){
                ss << charType(type);
            }
            charType(func->_return);
            std::string name = ss.str();
            func->name = name;
            charType(func->_return);
            func->fullName = charType(func->_return);
            func->fullName += name;

            return func;
        }

        SrcFile* parseProg(SrcFile* file){
            m_tokens = file->tokens;
            m_I = file->tokenPtr;
            while ((peak().has_value()))
            {
                if(auto stmt = parseStmt()){
                    file->stmts.push_back(stmt.value());
                }else if(auto func = parseFunc()){
                    file->funcs.reserve(1);
                    file->funcs.insert(std::pair(func.value()->name,func.value()));
                }else {
                    std::cerr << "Invalid Expression3" << std::endl;
                    exit(EXIT_FAILURE);
                }
            }
            file->tokenPtr = m_I;
            return file;
        }

        inline std::optional<Token> peak(int count = 0) const{
            if(m_I + count >= m_tokens.size())return {};
            else return m_tokens.at(m_I + count);
        }

        inline Token consume(){
            return m_tokens.at(m_I++);
        }

        inline Token tryConsume(TokenType type,const std::string& err){
            if(peak().has_value() && peak().value().type == type)return consume();
            else {
                std::cerr << err << "\nat line: " << peak().has_value() ? peak().value().line:peak(-1).value().line;
                std::cerr << std::endl;
                exit(EXIT_FAILURE);
            }
        }

        inline std::optional<Token> tryConsume(TokenType type){
            if(peak().has_value() && peak().value().type == type)return consume();
            else return {};
        }

    private:
        std::vector<Token> m_tokens;
        size_t m_I = 0;
        ArenaAlocator m_alloc;
};