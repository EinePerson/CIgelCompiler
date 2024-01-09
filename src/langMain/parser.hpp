#pragma once 

#include <vector>
#include <variant>
#include <iostream>

#include "codeGen/Generator.h"


class Generator;
struct Struct;

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
        explicit Parser()= default;

        std::optional<NodeTerm*> parseTerm(){
            NodeTerm* term;
            if(auto int_lit = tryConsume(TokenType::int_lit)){
                auto nodeInt = new NodeTermIntLit;
                nodeInt->int_lit = int_lit.value();
                nodeInt->sid = 2;//m_sidFlag;
                term = nodeInt;
            }else if(auto id = tryConsume(TokenType::id)){
                if(tryConsume(TokenType::openParenth)){
                    auto call = new NodeTermFuncCall;
                    call->name = id.value().value.value();
                    char i = 0;
                    while(auto expr = parseExpr()){
                        if(i)tryConsume(TokenType::comma,"Expected ','");
                        call->exprs.push_back(expr.value());
                        if(!tryConsume(TokenType::comma))i = 1;
                    }
                    tryConsume(TokenType::closeParenth,"Expected ')'");
                    term = call;
                }else if(peak().value().type == TokenType::openBracket){
                    auto arrAcc = new NodeTermArrayAcces;
                    arrAcc->id = id.value();
                    std::vector<NodeExpr*> ids {};
                    while (tryConsume(TokenType::openBracket)){
                        if(auto expr = parseExpr())ids.push_back(expr.value());
                        else{
                            std::cerr << "Expected Expression at " << peak(-1).value().file << ":" << peak(-1).value().line << std::endl;
                            exit(EXIT_FAILURE);
                        }
                        tryConsume(TokenType::closeBracket,"Expected ']'");
                    }
                    arrAcc->exprs = ids;
                    term = arrAcc;
                }else {
                    auto *nodeId = new NodeTermId;
                    nodeId->id = id.value();
                    term = nodeId;
                }
            }else if(auto paren = tryConsume(TokenType::openParenth)){
                auto expr = parseExpr();
                if(!expr.has_value()){
                    std::cerr << "Expected Expression" << "\nat line: " << paren.value().line << std::endl;
                    exit(EXIT_FAILURE);
                }
                tryConsume(TokenType::closeParenth,"Expected ')'");
                auto term_paren = new NodeTermParen;
                term_paren->expr = expr.value();
                term = term_paren;
            }else if(auto _new = tryConsume(TokenType::_new)) {
                auto cr = new NodeTermNew;
                cr->typeName = tryConsume(TokenType::id,"Expected Identifier").value.value();
                return cr;
            }else if(tryConsume(TokenType::null)) {
                return new NodeTermNull;
            }else
                return {};
            if(tryConsume(TokenType::connector)) {
                auto termT = new NodeTermStructAcces;
                termT->id = dynamic_cast<NodeTermId*>(term)->id;
                auto termF = peak();
                if(peak(1).has_value() && peak(1).value().type != TokenType::connector)consume();
                if(!termF)return term;
                termT->acc = termF.value();
                term = termT;
            }
            if(auto ro = parseTerm()) {
                NodeTerm* r = ro.value();
                r->contained = term;
                return r;
            }
            return term;
        }

        std::optional<NodeExpr*> parseExpr(int minPrec = 0){
            std::optional<NodeTerm*> termLs = parseTerm();
            if(!termLs.has_value())return {};
            NodeExpr* expr = termLs.value();

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
                if(op.type == TokenType::plus){
                    auto add = new NodeBinExprAdd;
                    add->ls = expr;
                    add->rs = exprR.value();
                    expr = add;
                }else if(op.type == TokenType::mul){
                    auto mul = new NodeBinExprMul;
                    mul->ls = expr;
                    mul->rs = exprR.value();
                    expr = mul;
                }else if(op.type == TokenType::div){
                    auto div = new NodeBinExprDiv;
                    div->ls = expr;
                    div->rs = exprR.value();
                    expr = div;
                }else if(op.type == TokenType::sub){
                    auto sub = new NodeBinExprSub;
                    sub->ls = expr;
                    sub->rs = exprR.value();
                    expr = sub;
                }else if(op.type >= TokenType::equal && op.type <= TokenType::small){
                    auto areth = new NodeBinAreth;
                    areth->ls = expr;
                    areth->rs = exprR.value();
                    areth->type = op.type;
                    expr = areth;
                }else{
                    std::cerr << "Unkown operator" << std::endl;
                    exit(EXIT_FAILURE);
                }
            }
            return expr;
        }

        std::optional<NodeStmtScope*> parseScope(){
            if(!tryConsume(TokenType::openCurl).has_value())return {};
            auto scope = new NodeStmtScope;
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
                auto* stmt = new NodeStmtExit;
                if(auto nodeExpr = parseExpr()){
                    stmt->expr = nodeExpr.value();
                }else{
                    std::cerr << "Invalid Expression1" << std::endl;
                    exit(EXIT_FAILURE);
                }
                tryConsume(TokenType::closeParenth, "Expected `)`");
                tryConsume(TokenType::semi,"Expected ';'");
                return stmt;
            }else if(peak().has_value() && peak().value().type <= TokenType::_ulong){
                if(peak(1).has_value() && peak(1).value().type == TokenType::id && peak(2).has_value()
                && (peak(2).value().type == TokenType::eq || peak(2).value().type == TokenType::semi)) {
                    auto pirim = new NodeStmtPirimitiv;
                    pirim->type = getType(peak().value().type);
                    pirim->sid = (char) peak().value().type;
                    m_sidFlag = pirim->sid;
                    pirim->_signed = consume().type <= TokenType::_long;
                    pirim->id = consume();
                    if(!peak().has_value()){
                        //TODO Exit on error
                    }
                    if(peak().value().type == TokenType::semi){
                        consume();
                        auto intLit = new NodeTermIntLit;
                        Token tok{.type = TokenType::int_lit,.value = "0"};
                        intLit->int_lit = tok;
                        pirim->expr = intLit;
                    } else {
                        tryConsume(TokenType::eq,"Expected '='");
                        if (auto expr = parseExpr())
                            pirim->expr = expr.value();
                        else {
                            std::cerr << "Unexpected expression1" << std::endl;
                            exit(EXIT_FAILURE);
                        }
                        tryConsume(TokenType::semi, "Expected ';'");
                    }
                    return pirim;
                }else if(peak(1).has_value() && peak(1).value().type == TokenType::openBracket){
                    auto arr = new NodeStmtArr;
                    arr->sid = (char) peak().value().type;
                    arr->_signed = consume().type <= TokenType::_long;
                    if(peak(1).has_value() && peak(1).value().type == TokenType::int_lit){
                        while (peak().has_value() && peak().value().type == TokenType::openBracket && peak(1).has_value() && peak(1).value().type == TokenType::int_lit &&
                        peak(2).has_value() && peak(2).value().type == TokenType::closeBracket){
                            consume();
                            arr->size.push_back(stoi(tryConsume(TokenType::int_lit,"Expected Integer as value").value.value()));
                            consume();
                        }
                        arr->fixed = true;
                    }else{
                        while (peak().has_value() && peak().value().type == TokenType::openBracket && peak(1).has_value() && peak(1).value().type == TokenType::closeBracket){
                            consume();
                            arr->size.push_back(-1);
                            consume();
                        }
                        arr->fixed = false;
                    }

                    arr->id = consume();
                    if(!arr->fixed && tryConsume(TokenType::eq)) {
                        //TODO add new n stuff
                    }
                    tryConsume(TokenType::semi, "Expected ';'");
                    return arr;
                }
            }else if(auto scope = parseScope()){
                return scope.value();
            }else if(auto _if = tryConsume(TokenType::_if)){
                return parseIf();
            }else if(peak().has_value() && peak().value().type == TokenType::id && peak(1).has_value() && peak(1).value().type == TokenType::id &&
                peak(2).has_value() && (peak(2).value().type == TokenType::eq || peak(2).value().type == TokenType::semi)/*&& peak(3).has_value() && peak(3).value().type == TokenType::_new*/) {
                auto id = tryConsume(TokenType::id);
                auto strNew = new NodeStmtStructNew;
                strNew->typeName = id.value().value.value();
                strNew->id = tryConsume(TokenType::id,"Expected name");
                if(tryConsume(TokenType::eq)) {
                    if(auto t = parseTerm())
                        strNew->term = t.value();
                    else {
                        //TODO throw error
                    }
                }else {
                    strNew->term = new NodeTermNull;
                }
                tryConsume(TokenType::semi,"Expected ';'");
                return strNew;
            } else if(auto id = /*tryConsume(TokenType::id)*/ parseTerm()){
                if(auto val = dynamic_cast<NodeTermFuncCall*>(id.value())){
                    auto* call = new NodeStmtFuncCall;
                    call->call = val;
                    return call;
                }else if(auto value = dynamic_cast<NodeTermArrayAcces *>(id.value())){
                    auto reassign = new NodeStmtArrReassign;
                    reassign->acces = value;
                    if (peak().has_value() && peak().value().type >= TokenType::eq &&
                            peak().value().type <= TokenType::pow_eq) {
                        reassign->op = consume().type;

                        if (auto expr = parseExpr())reassign->expr = expr.value();
                        else {
                            std::cerr << "Unexpected expression2" << std::endl;
                            exit(EXIT_FAILURE);
                        }
                    } else if (peak().has_value() && peak().value().type == TokenType::inc ||
                                       peak().value().type == TokenType::dec) {
                        reassign->op = consume().type;
                    } else {
                        std::cerr << "Expected Assignment Operator1" << std::endl;
                        exit(EXIT_FAILURE);
                    }
                    tryConsume(TokenType::semi, "Expected ';'");
                    return reassign;
                }else if(peak().has_value() && (peak().value().type == TokenType::inc || peak().value().type == TokenType::dec)){
                    auto reassign = new NodeStmtReassign;
                    reassign->op = consume().type;
                    reassign->id = id.value();

                    tryConsume(TokenType::semi,"Expected ';'");
                    return  reassign;
                }else{
                        auto reassign = new NodeStmtReassign;
                        reassign->id = id.value();
                        if (peak().has_value() && peak().value().type >= TokenType::eq &&
                            peak().value().type <= TokenType::pow_eq) {
                            reassign->op = consume().type;

                            if (auto expr = parseExpr())reassign->expr = expr.value();
                            else {
                                std::cerr << "Unexpected expression2" << std::endl;
                                exit(EXIT_FAILURE);
                            }
                        } else if (peak().has_value() && peak().value().type == TokenType::inc ||
                                   peak().value().type == TokenType::dec) {
                            reassign->op = consume().type;
                        } else {
                            std::cerr << "Expected Assignment Operator2" << std::endl;
                            exit(EXIT_FAILURE);
                        }
                        tryConsume(TokenType::semi, "Expected ';'");
                        return reassign;
                }
            }else if(peak().has_value() && peak().value().type == TokenType::_return){
                consume();
                auto _return = new Return;
                if(auto expr = parseExpr()){
                    _return->val = expr.value();
                }
                tryConsume(TokenType::semi,"Expected ';'");
                return _return;
            }
                return {};
        }

        NodeStmtIf* parseIf(){
            tryConsume(TokenType::openParenth,"Expected '('");
            auto stmt_if = new NodeStmtIf;
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

        std::optional<IgFunction*> parseFunc(){
            auto func = new IgFunction;
            if(peak().has_value() && (peak().value().type <= TokenType::_ulong || peak().value().type == TokenType::_void))func->_return = getType(consume().type);
            else return {};

            auto hname = tryConsume(TokenType::id,"Expected identifier").value;

            tryConsume(TokenType::openParenth,"Expected '('");
            char i = 0;
            while (peak().has_value() && peak().value().type <= TokenType::_ulong)
            {
                if(i)tryConsume(TokenType::comma,"Expected ','");
                func->paramType.push_back(getType(consume().type));
                func->paramName.push_back(tryConsume(TokenType::id,"Expected identifier").value.value());
                if(!tryConsume(TokenType::comma))i = 1;
            }

            tryConsume(TokenType::closeParenth,"Expected ')'");
            if(auto scope = parseScope())func->scope = scope.value();
            else {
                std::cerr << "Expected Scope" << std::endl;
                exit(EXIT_FAILURE);
            }

            func->name = hname.value();

            return func;
        }

        std::optional<IgType*> parseType() {
            if(peak(1).value().type != TokenType::id) return  {};
            switch (peak().value().type) {
                case TokenType::_struct: {
                    auto str = new Struct;
                    consume();
                    str->name = consume().value.value();
                    NodeStmtScope* scope = parseScope().value();
                    for (auto stmt : scope->stmts) {
                        if(auto val = dynamic_cast<NodeStmtLet *>(stmt)) {
                            str->vars.push_back(val);
                            str->types.push_back(val->type);
                        }else {
                            std::cerr << "Only Fields/Attributes/Variables are allowed in structs\n";
                            exit(EXIT_FAILURE);
                        }
                    }
                    return str;
                }
                //break;
                default:
                    return {};
            }
        }

        SrcFile* parseProg(SrcFile* file){
            m_tokens = file->tokens;
            m_I = file->tokenPtr;
            while ((peak().has_value()))
            {
                if(auto stmt = parseStmt()){
                    file->stmts.push_back(stmt.value());
                }else if(auto func = parseFunc()) {
                    file->funcs.insert(std::pair(FuncSig(func.value()->name,func.value()->paramType),func.value()));
                }else if(auto type = parseType()){
                    file->types.push_back(type.value());
                }else {
                    std::cerr << "Invalid Expression3" << std::endl;
                    exit(EXIT_FAILURE);
                }
            }
            file->tokenPtr = m_I;
            return file;
        }

        [[nodiscard]] std::optional<Token> peak(int count = 0) const{
            if(m_I + count >= m_tokens.size())return {};
            else return m_tokens.at(m_I + count);
        }

        Token consume(){
            return m_tokens.at(m_I++);
        }

        Token tryConsume(TokenType type,const std::string& err){
            if(peak().has_value() && peak().value().type == type)return consume();
            else {
                std::cerr << err << "\nat line: " << (peak().has_value() ? peak().value().line:peak(-1).value().line);
                std::cerr << std::endl;
                exit(EXIT_FAILURE);
            }
        }

        std::optional<Token> tryConsume(TokenType type){
            if(peak().has_value() && peak().value().type == type)return consume();
            else return {};
        }

    private:
        std::vector<Token> m_tokens;
        size_t m_I = 0;
        char m_sidFlag = -1;
};