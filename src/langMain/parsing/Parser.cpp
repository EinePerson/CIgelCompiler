//
// Created by igel on 12.01.24.
//

#include "Parser.h"
#include "../../types.h"

#include <llvm/Support/CommandLine.h>

std::optional<NodeTerm *> Parser::parseTerm(std::optional<BeContained*> contP,NodeTerm* term) {
            if(auto int_lit = tryConsume(TokenType::int_lit)){
                auto nodeInt = new NodeTermIntLit;
                nodeInt->int_lit = int_lit.value();
                char sid = sidChar(int_lit.value().value.value().back());
                if(sid == -1) {
                    if(m_sidFlag >= 0) {
                        nodeInt->sid = m_sidFlag;
                        m_sidFlag = -1;
                    }else nodeInt->sid = 2;
                }else nodeInt->sid = 2;
                nodeInt->_signed = nodeInt->sid <= 3;
                term = nodeInt;
            }else if(auto str = tryConsume(TokenType::str)) {
                auto nodestr = new NodeTermStringLit;
                nodestr->str = str.value();
                term = nodestr;
            }else if((peak().has_value() && peak().value().type == TokenType::id) || contP.has_value()){
                BeContained* cont = contP.has_value()?contP.value():parseContained().value();
                if(tryConsume(TokenType::openParenth)){
                    auto call = new NodeTermFuncCall;
                    call->name = cont;
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
                    arrAcc->cont = cont;
                    std::vector<NodeExpr*> ids {};
                    while (tryConsume(TokenType::openBracket)){
                        if(auto expr = parseExpr())ids.push_back(expr.value());
                        else{
                            err("Expected Expression");
                        }
                        tryConsume(TokenType::closeBracket,"Expected ']'");
                    }
                    arrAcc->exprs = ids;
                    term = arrAcc;
                }else {
                    auto *nodeId = new NodeTermId;
                    nodeId->cont = cont;
                    term = nodeId;
                }
            }else if(auto paren = tryConsume(TokenType::openParenth)){
                auto expr = parseExpr();
                if(!expr.has_value()){
                    err("Expected Expression");
                }
                tryConsume(TokenType::closeParenth,"Expected ')'");
                auto term_paren = new NodeTermParen;
                term_paren->expr = expr.value();
                term_paren->_signed = expr.value()->_signed;
                term = term_paren;
            }else if(auto _new = tryConsume(TokenType::_new)) {
                if(peak().has_value() && peak().value().type <= TokenType::_bool) {
                    auto* arr = new NodeTermArrNew;
                    arr->sid = static_cast<char>(consume().type);
                    while (peak().has_value() && peak().value().type == TokenType::openBracket){
                        tryConsume(TokenType::openBracket,"Expected '['");
                        if(auto cont = parseTerm()) {
                            arr->size.push_back(cont.value());
                        }else err("Expected Term");
                        tryConsume(TokenType::closeBracket,"Expected ']'");
                    }
                    arr->contained = nullptr;
                    arr->floating = arr->sid == static_cast<char>(TokenType::_float) || arr->sid == static_cast<char>(TokenType::_double);
                    arr->_signed = arr->sid <= static_cast<char>(TokenType::_long);
                    return  arr;
                }else if(auto typeName = parseContained()){
                    if(peak().has_value() && peak().value().type == TokenType::openBracket) {
                        auto* arr = new NodeTermArrNew;
                        arr->sid = -1;
                        arr->typeName = typeName.value();
                        while (peak().has_value() && peak().value().type == TokenType::openBracket){
                            tryConsume(TokenType::openBracket,"Expected '['");
                            if(auto cont = parseTerm()) {
                                arr->size.push_back(cont.value());
                            }else err("Expected Term");
                            tryConsume(TokenType::closeBracket,"Expected ']'");
                        }
                        arr->contained = nullptr;
                        arr->floating = arr->sid == static_cast<char>(TokenType::_float) || arr->sid == static_cast<char>(TokenType::_double);
                        arr->_signed = arr->sid <= static_cast<char>(TokenType::_long);
                        return  arr;
                    }else if(dynamic_cast<Class*>(typeName.value())) {
                        auto _nCz = new NodeTermClassNew;
                        _nCz->typeName = typeName.value();
                        tryConsume(TokenType::openParenth,"Expected '(' after new Class");
                        char i = 0;
                        while(auto expr = parseExpr()){
                            if(i)tryConsume(TokenType::comma,"Expected ','");
                            _nCz->exprs.push_back(expr.value());
                            if(!tryConsume(TokenType::comma))i = 1;
                        }
                        tryConsume(TokenType::closeParenth,"Expected ')'");
                        term = _nCz;
                    }
                }else err("Expected type name");
            } else if(tryConsume(TokenType::null)) {
                return new NodeTermNull;
            }
            if(tryConsume(TokenType::connector)) {
                auto termT = new NodeTermAcces;
                bool length = false;
                if(auto val = dynamic_cast<NodeTermId*>(term)) {
                    termT->id = val->cont;

                }
                if(auto val = dynamic_cast<NodeTermFuncCall*>(term)) termT->call = val;
                if(dynamic_cast<NodeTermArrayAcces*>(term)) {
                    if(peak().has_value() && peak().value().type == TokenType::id && peak().value().value.value() == "length") {
                        length = true;
                    }
                    termT->contained = term;
                }

                if(length) {
                    auto len = new NodeTermArrayLength;
                    len->arr = dynamic_cast<NodeTermArrayAcces*>(term);
                    term = len;
                }else {
                    auto termF = peak();
                    if(peak(1).has_value() && peak(1).value().type != TokenType::connector && peak(1).value().type != TokenType::openBracket && peak(1).value().type != TokenType::openParenth)consume();
                    if(!termF)return term;
                    termT->acc = termF.value();
                    term = termT;
                }
            }/*else if(tryConsume(TokenType::ptrConnect)) {
                auto termT = new NodeTermClassAcces;
                bool length = false;
                if(auto val = dynamic_cast<NodeTermId*>(term))
                    termT->id = val->cont;
                if(auto val = dynamic_cast<NodeTermFuncCall*>(term)) termT->call = val;
                if(dynamic_cast<NodeTermArrayAcces*>(term)) {
                    if(peak().has_value() && peak().value().type == TokenType::id && peak().value().value.value() == "length") {
                        length = true;
                    }
                    termT->contained = term;
                }

                if(length) {
                    auto len = new NodeTermArrayLength;
                    len->arr = dynamic_cast<NodeTermArrayAcces*>(term);
                    term = len;
                }else {
                    auto termF = peak();
                    if(peak(1).has_value() && peak(1).value().type != TokenType::ptrConnect && peak(1).value().type != TokenType::openBracket && peak(1).value().type != TokenType::openParenth)consume();
                    if(!termF)return term;
                    termT->acc = termF.value();
                    term = termT;
                }
            }*/
            if(!term)return {};
            if(auto ro = parseTerm()) {
                NodeTerm* r = ro.value();
                r->contained = term;
                return r;
            }
            return term;
        }

        std::optional<NodeExpr*> Parser::parseExpr(int minPrec){
            NodeBinNeg* neg = nullptr;
            if(tryConsume(TokenType::_not))neg = new NodeBinNeg;
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
                    err("Unable to parse");
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
                }else if(op.type >= TokenType::_bitOr && op.type <= TokenType::_bitAnd) {
                    auto bit = new NodeBinBit;
                    bit->ls = expr;
                    bit->rs = exprR.value();
                    bit->op = op.type;
                    expr = bit;
                }else{
                    err("Unkown operator");
                }
                expr->_signed = dynamic_cast<NodeBinExpr*>(expr)->ls->_signed && dynamic_cast<NodeBinExpr*>(expr)->rs->_signed;
            }
            if(neg) {
                neg->expr = expr;
                return neg;
            }
            return expr;
        }

        std::optional<NodeStmtScope*> Parser::parseScope(){
            if(!tryConsume(TokenType::openCurl).has_value())return {};
            auto scope = new NodeStmtScope;
            varTypes.emplace_back();
            while(auto stmt = parseStmt()){
                scope->stmts.push_back(stmt.value());
            }
            varTypes.pop_back();
            tryConsume(TokenType::closeCurl,"Expected '}'");
            return scope;
        }

        std::optional<NodeStmt*> Parser::parseStmt(bool semi){
            bool final = tryConsume(TokenType::final).has_value();
            bool _static = tryConsume(TokenType::_static).has_value();
            if(peak().has_value() && peak().value().type <= TokenType::_bool){
                if(auto var = parsePirim(_static,final))return var.value();
                else err("Expected variable declaration");
            }else if(auto scope = parseScope()){
                return scope.value();
            }else if(auto _if = tryConsume(TokenType::_if)){
                return parseIf();
            }else if(auto _for = tryConsume(TokenType::_for)) {
                return parseFor();
            }else if(auto _while = tryConsume(TokenType::_while)) {
                return parseWhile();
            }else if(auto _try = tryConsume(TokenType::_try)) {
                auto stmtTry = new NodeStmtTry;
                if(auto scope = parseScope()) {
                    stmtTry->scope = scope.value();
                }else {
                    err("Expected Scope after 'try'");
                }
                uint _catch = 0;
                while(auto _catchT = tryConsume(TokenType::_catch)) {
                    _catch++;
                    tryConsume(TokenType::openParenth,"Expected '('");
                    auto _catchCl = new NodeStmtCatch;
                    if(auto cont = parseContained()) {
                        _catchCl->typeName = cont.value();
                    }else {
                        err("Expected Type name after 'catch'");
                    }

                    _catchCl->varName = tryConsume(TokenType::id,"Expected identifier").value.value();

                    tryConsume(TokenType::closeParenth,"Expected ')'");
                    if(auto scope = parseScope()) {
                        _catchCl->scope = scope.value();
                    }else {
                        err("Expected Scope after 'catch'");
                    }
                    stmtTry->catch_.push_back(_catchCl);
                }

                if(_catch == 0)err("At least one catch clause is required after a try");
                return stmtTry;
            }else if(tryConsume(TokenType::_throw)) {
                auto _throw = new NodeStmtThrow;
                if(auto expr = parseExpr()) {
                    _throw->expr = expr.value();
                }else {
                    err("Expected Expression after 'throw'");
                }
                tryConsume(TokenType::semi,"Expected ';'");
                return _throw;
            }else if(auto _switchT = tryConsume(TokenType::_switch)) {
                auto* _switch = new NodeStmtSwitch;
                tryConsume(TokenType::openParenth,"Expected '('");
                if(auto term = parseTerm()) {
                    _switch->cond = term.value();
                }else err("Expected Term");
                tryConsume(TokenType::closeParenth,"Expected ')'");
                if(auto scope = parseScope()) {
                    _switch->scope = scope.value();
                }else err("Expected Scope");
                return _switch;
            }else if(auto _caseT = tryConsume(TokenType::_case)) {
                auto* _case = new NodeStmtCase;
                if(auto lit = parseTerm()) {
                    if(auto termLit = dynamic_cast<NodeTermIntLit*>(lit.value())) {
                        _case->cond = termLit;
                        tryConsume(TokenType::next,"Expected ':'");
                        return _case;
                    }
                }
                err("Expected number");
            }else if(auto _caseT = tryConsume(TokenType::_default)) {
                auto* _case = new NodeStmtCase;
                tryConsume(TokenType::next,"Expected ':'");
                _case->_default = true;
                return _case;
            }else if(peak().has_value() && peak().value().type == TokenType::_return){
                consume();
                auto _return = new NodeStmtReturn;
                if(auto expr = parseExpr()){
                    _return->val = expr.value();
                }
                tryConsume(TokenType::semi,"Expected ';'");
                return _return;
            }else if(peak().has_value() && peak().value().type == TokenType::_break) {
                consume();
                tryConsume(TokenType::semi,"Expected ';'");
                return new NodeStmtBreak;
            }else if(peak().has_value() && peak().value().type == TokenType::_continue) {
                consume();
                tryConsume(TokenType::semi,"Expected ';'");
                return new NodeStmtContinue;
            }else if(peak().has_value() && peak().value().type == TokenType::id) {
                BeContained* cont;
                if(auto co = parseContained())cont = co.value();
                else err("Could not parse contained");
                if(dynamic_cast<IgType*>(cont)) {
                    if(auto let = parseLet(_static,final,cont))return let.value();
                    err("Expected variable declaration");
                    return {};
                }
                auto id = parseTerm(cont);
                if(id.has_value()){
                    if(auto val = dynamic_cast<NodeTermFuncCall*>(id.value())) {
                        auto* call = new NodeStmtFuncCall;
                        call->call = val;
                        tryConsume(TokenType::semi, "Expected ';'");
                        return call;
                    }else if(auto value = dynamic_cast<NodeTermArrayAcces *>(id.value())) {
                        auto reassign = new NodeStmtArrReassign;
                        reassign->acces = value;
                        if (peak().has_value() && peak().value().type >= TokenType::eq &&
                                peak().value().type <= TokenType::pow_eq) {
                            reassign->op = consume().type;

                            if (auto expr = parseExpr())reassign->expr = expr.value();
                            else {
                                err("Unexpected expression2");
                            }
                                } else if (peak().has_value() && peak().value().type == TokenType::inc ||peak().value().type == TokenType::dec) {
                                    reassign->op = consume().type;
                                } else {
                                    err("Expected Assignment Operator1");
                                }
                        tryConsume(TokenType::semi, "Expected ';'");
                        return reassign;
                    }else {
                        auto reassign = new NodeStmtReassign;
                        reassign->id = id.value();//dynamic_cast<Name*>(cont)->getId();
                        if (peak().has_value() && peak().value().type >= TokenType::eq && peak().value().type <= TokenType::pow_eq) {
                            reassign->op = consume().type;

                            if (auto expr = parseExpr())reassign->expr = expr.value();
                            else err("Unexpected expression2");
                        }else if (peak().has_value() && peak().value().type == TokenType::inc ||peak().value().type == TokenType::dec) {
                            reassign->op = consume().type;
                        } else {
                            err("Expected Assignment Operator2");
                        }
                        if(semi)tryConsume(TokenType::semi, "Expected ';'");
                        return reassign;
                    }
                    return {};
                }else if(peak().has_value() && (peak().value().type == TokenType::inc || peak().value().type == TokenType::dec)){
                    auto reassign = new NodeStmtReassign;
                    reassign->op = consume().type;
                    reassign->id = dynamic_cast<Name*>(cont)->getId();

                    if(semi)tryConsume(TokenType::semi,"Expected ';'");
                    return  reassign;
                }else {
                    auto reassign = new NodeStmtReassign;
                    reassign->id = dynamic_cast<Name*>(cont)->getId();
                    if (peak().has_value() && peak().value().type >= TokenType::eq && peak().value().type <= TokenType::pow_eq) {
                        reassign->op = consume().type;

                        if (auto expr = parseExpr())reassign->expr = expr.value();
                        else err("Unexpected expression2");
                    }else if (peak().has_value() && peak().value().type == TokenType::inc ||peak().value().type == TokenType::dec) {
                        reassign->op = consume().type;
                    } else {
                        err("Expected Assignment Operator2");
                    }
                    if(semi)tryConsume(TokenType::semi, "Expected ';'");
                    return reassign;
                }
            }
            return {};
        }

std::optional<NodeStmtLet*> Parser::parseLet(bool _static, bool final, std::optional<BeContained*> contP) {
    if(contP.has_value()) {
        if(peak().has_value() && peak().value().type == TokenType::openBracket)return parseArrNew(_static,final,contP);
        return parseNew(_static,final,contP);
    }
    if(peak().value().type <= TokenType::_bool)return parsePirim(_static,final);
    BeContained* cont;
    if(auto co = parseContained())cont = co.value();
    else return {};
    if(peak().has_value() && peak().value().type == TokenType::openBracket)return parseArrNew(_static,final,cont);
    return parseNew(_static,final,cont);
}

std::optional<NodeStmtPirimitiv*> Parser::parsePirim(bool _static, bool final) {
    if(peak().has_value() && peak().value().type <= TokenType::_bool && peak(1).has_value() && peak(1).value().type == TokenType::id && peak(2).has_value()
                && (peak(2).value().type == TokenType::eq || peak(2).value().type == TokenType::semi)) {
        auto pirim = new NodeStmtPirimitiv;
        pirim->type = getType(peak().value().type);
        pirim->sid = static_cast<char>(peak().value().type);
        m_sidFlag = pirim->sid;
        pirim->_signed = consume().type <= TokenType::_long;
        pirim->name = consume().value.value();
        if(!peak().has_value()){
            err("Expected expression or '='");
        }
        if(peak().value().type == TokenType::semi){
            consume();
            auto intLit = new NodeTermIntLit;
            Token tok(TokenType::int_lit,-1,-1,"","0");
            intLit->_signed = false;
            intLit->int_lit = tok;
            pirim->expr = intLit;
        } else {
            tryConsume(TokenType::eq,"Expected '='");
            if (auto expr = parseExpr())pirim->expr = expr.value();
            else err("Unexpected expression1");
            tryConsume(TokenType::semi, "Expected ';'");
        }
        pirim->final = final;
        pirim->_static = _static;
        if(varHolder)pirim->contType = varHolder;
        varTypes.back()[pirim->name] = Types::VarType::PirimVar;
        return pirim;
    }
    return {};
}

std::optional<NodeStmtNew*> Parser::parseNew(bool _static, bool final,std::optional<BeContained*> contP) {
    IgType* cont;
    if(auto val = dynamic_cast<IgType*>(contP.has_value()?contP.value():parseContained().value()))cont = val;
    else return {};
    Token varName = tryConsume(TokenType::id,"Expected name");
    if(dynamic_cast<Struct*>(cont)) {
        if(peak().value().type == TokenType::semi) {
            auto strNew = new NodeStmtStructNew;
            strNew->type = reinterpret_cast<Type *>(2);
            strNew->typeName = cont;
            if(cont->contType.has_value())strNew->contType = cont->contType.value();
            strNew->name = varName.value.value();
            consume();
            strNew->final = final;
            strNew->_static = _static;
            if(varHolder)strNew->contType = varHolder;
            varTypes.back()[strNew->name] = Types::VarType::StructVar;
            return strNew;
        }
    }else if(dynamic_cast<Class*>(cont)) {
        if(peak().value().type == TokenType::eq) {
            auto strNew = new NodeStmtClassNew;
            strNew->typeName = cont;
            if(cont->contType.has_value())strNew->contType = cont->contType;
            if(cont->contType.has_value())strNew->contType = cont->contType.value();
            strNew->name = varName.value.value();
            tryConsume(TokenType::eq,"Expected '='");
            if(auto t = parseTerm())
                strNew->term = t.value();
            else err("Expected Term after '='");
            tryConsume(TokenType::semi,"Expected ';'");
            strNew->final = final;
            strNew->_static = _static;
            if(varHolder)strNew->contType = varHolder;
            varTypes.back()[strNew->name] = Types::VarType::ClassVar;
            return strNew;
        }
        if(peak().value().type == TokenType::semi) {
            auto strNew = new NodeStmtClassNew;
            strNew->type = reinterpret_cast<Type *>(2);
            strNew->typeName = cont;
            if(cont->contType.has_value())strNew->contType = cont->contType.value();
            strNew->name = varName.value.value();
            strNew->term = new NodeTermNull;
            consume();
            strNew->final = final;
            strNew->_static = _static;
            if(varHolder)strNew->contType = varHolder;
            varTypes.back()[strNew->name] = Types::VarType::ClassVar;
            return strNew;
        }
    }
    return {};
}

std::optional<NodeStmtArr*> Parser::parseArrNew(bool _static, bool final,std::optional<BeContained*> contP) {
    if(peak(1).has_value() && peak(1).value().type == TokenType::openBracket){
        auto arr = new NodeStmtArr;
        arr->sid = (char) peak().value().type;
        arr->_signed = consume().type <= TokenType::_long;
        arr->type = reinterpret_cast<Type *>(1);
        while (peak().has_value() && peak().value().type == TokenType::openBracket && peak(1).has_value() && peak(1).value().type == TokenType::closeBracket){
            consume();
            arr->size++;
            consume();
        }
        arr->fixed = false;
        arr->name = consume().value.value();
        if(!arr->fixed && tryConsume(TokenType::eq)) {
            if(auto t = parseTerm()) {
                arr->term = t.value();
            }else err("Expected Term");
        }
        tryConsume(TokenType::semi, "Expected ';'");
        arr->final = final;
        arr->_static = _static;
        if(varHolder)arr->contType = varHolder;
        varTypes.back()[arr->name] = Types::VarType::ArrayVar;
        return arr;
    }
    IgType* cont;
    if(auto val = dynamic_cast<IgType*>(contP.has_value()?contP.value():parseContained().value()))cont = val;
    else return {};

    if(peak().value().type == TokenType::openBracket) {
        auto* arr = new NodeStmtArr;

        arr->typeName = cont;

        arr->fixed = false;
        arr->_signed = false;
        arr->sid = -1;
        arr->type = reinterpret_cast<Type *>(1);
        while (peak().has_value() && peak().value().type == TokenType::openBracket && peak(1).has_value() && peak(1).value().type == TokenType::closeBracket){
            consume();
            arr->size++;
            consume();
        }
        arr->name = consume().value.value();
        if(!arr->fixed && tryConsume(TokenType::eq)) {
            if(auto t = parseTerm()) {
                arr->term = t.value();
            }else err("Expected Term");
        }
        tryConsume(TokenType::semi, "Expected ';'");
        arr->final = final;
        arr->_static = _static;
        if(varHolder)arr->contType = varHolder;
        varTypes.back()[arr->name] = Types::VarType::ArrayVar;
        return arr;
    }
    return {};
}

NodeStmtFor* Parser::parseFor() {
            tryConsume(TokenType::openParenth,"Expected '('");
            auto stmt_for = new NodeStmtFor;

            auto stmt_let = parseStmt();
            if(stmt_let.has_value()) {
                if(auto stmt = dynamic_cast<NodeStmtLet*>(stmt_let.value())) {
                    stmt_for->let = stmt;
                }else {
                    err("Expected Variable Assignment");
                }
            }else tryConsume(TokenType::semi,"Expected ';'");

            int i = 0;
            while(auto expr = parseExpr()){
                i++;
                stmt_for->expr.push_back(expr.value());
                if(auto cond = tryConsume(TokenType::_and))stmt_for->exprCond.push_back(cond.value().type);
                else if(auto cond = tryConsume(TokenType::_or))stmt_for->exprCond.push_back(cond.value().type);
                else if(auto cond = tryConsume(TokenType::_xor))stmt_for->exprCond.push_back(cond.value().type);
            }
            if(!i){
                err("Invalid Expression2");
            }

            tryConsume(TokenType::semi,"Expected ';'");

            auto stmt_res = parseStmt(false);
            if(stmt_res.has_value()) {
                if(auto stmt = dynamic_cast<NodeStmtReassign*>(stmt_res.value())) {
                    stmt_for->res = stmt;
                }else {
                    err("Expected Variable Assignment");
                }
            }
            tryConsume(TokenType::closeParenth,"Expected ')'");
            if(auto scope = parseScope())stmt_for->scope = scope.value();
            else if(auto stmt = parseStmt())stmt_for->scope = stmt.value();
            else {
                err("Expected Scope or Statement");
            }
            return stmt_for;
        }

        NodeStmtWhile* Parser::parseWhile() {
            tryConsume(TokenType::openParenth,"Expected '('");
            auto stmt_while = new NodeStmtWhile;
            int i = 0;
            while(auto expr = parseExpr()){
                i++;
                stmt_while->expr.push_back(expr.value());
                if(auto cond = tryConsume(TokenType::_and))stmt_while->exprCond.push_back(cond.value().type);
                else if(auto cond = tryConsume(TokenType::_or))stmt_while->exprCond.push_back(cond.value().type);
                else if(auto cond = tryConsume(TokenType::_xor))stmt_while->exprCond.push_back(cond.value().type);
            }
            if(!i){
                err("Invalid Expression2");
            }
            tryConsume(TokenType::closeParenth,"Expected ')'");
            if(auto scope = parseScope())stmt_while->scope = scope.value();
            else if(auto stmt = parseStmt())stmt_while->scope = stmt.value();
            else {
                err("Expected Scope or Statement");
            }
            return stmt_while;
        }

        NodeStmtIf* Parser::parseIf(){
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
                err("Invalid Expression2");
            }
            tryConsume(TokenType::closeParenth,"Expected ')'2");
            if(auto scope = parseScope()){
                stmt_if->scope = scope.value();
            }else if(auto stms = parseStmt()){
                stmt_if->scope = stms.value();
            }else {
                err("Invalid Scope");
            }

            if(tryConsume(TokenType::_else)){
                if(tryConsume(TokenType::_if)){
                    stmt_if->else_if = parseIf();
                }else if(auto scope = parseScope()){
                    stmt_if->scope_else = scope.value();
                }else if(auto stms = parseStmt()){
                    stmt_if->scope_else = stms.value();
                }else {
                    err("Invalid else Scope");
                }
            }
            return stmt_if;
        }

        std::optional<IgFunction*> Parser::parseFunc(){
            auto func = new IgFunction;
            func->final = tryConsume(TokenType::final).has_value();
            func->_static = tryConsume(TokenType::_static).has_value();
            if(!m_super.empty()) {
                func->contType = m_super.back();
                m_super.back()->contained.push_back(func);
            }else func->_static = true;

            if(peak().has_value() && (peak().value().type <= TokenType::_ulong || peak().value().type == TokenType::_void || peak().value().type == TokenType::id)) {
                Token ret = consume();
                if(!ret.value.has_value())func->retTypeName = ret.value.value();
                else func->retTypeName = "";
                func->_return = getType(ret.type);
                func->returnSigned = false;
            }
            else if(peak().has_value() && (peak().value().type == TokenType::_float || peak().value().type == TokenType::_double)) {
                func->_return = getType(consume().type);
                func->returnSigned = true;
            }else return {};

            auto hname = tryConsume(TokenType::id,"Expected identifier").value;

            tryConsume(TokenType::openParenth,"Expected '('");
            char i = 0;
            while (peak().has_value() && (peak().value().type <= TokenType::_bool || peak().value().type == TokenType::id)) {
                if(i)tryConsume(TokenType::comma,"Expected ','");
                func->signage.push_back(peak().value().type <= TokenType::_long);
                Token param;
                BeContained* cont = nullptr;
                if(peak().has_value() && peak().value().type != TokenType::id)param = consume();
                else cont = parseContained().value();
                if(cont) {
                    func->paramType.push_back(PointerType::get(*Generator::m_contxt,0));
                    func->paramTypeName.push_back(cont);
                }else {
                    func->paramType.push_back(getType(param.type));
                    func->paramTypeName.emplace_back(nullptr);
                }
                func->paramName.push_back(tryConsume(TokenType::id,"Expected identifier").value.value());
                if(!tryConsume(TokenType::comma))i = 1;
            }

            tryConsume(TokenType::closeParenth,"Expected ')'");
            if(auto scope = parseScope())func->scope = scope.value();
            else {
                err("Expected Scope");
            }

            func->name = hname.value();

            return func;
        }

std::optional<BeContained*> Parser::parseContained() {
    //if(peak(1).value().type != TokenType::dConnect)return {};
    IgType* cont = nullptr;
    while (peak().value().type == TokenType::id && peak(1).value().type == TokenType::dConnect) {
        IgType* contN = m_file->nameTypeMap[consume().value.value()];
        if(cont)contN->contType = cont;
        cont = contN;
        consume();
        //consume();
    }
    if(m_file->nameTypeMap.contains(peak().value().value.value())) {
        IgType* contN = m_file->nameTypeMap[consume().value.value()];
        if(cont)contN->contType = cont;
        return contN;
    }
    Name* name = new Name("");
    name->contType = cont;
    name->name = tryConsume(TokenType::id,"Expected identifier").value.value();
    return name;
}

std::optional<IgType*> Parser::parseType() {
            if(peak(1).value().type != TokenType::id) return  {};
            switch (consume().type) {
                case TokenType::_struct: {
                    Struct* str = dynamic_cast<Struct *>(m_file->nameTypeMap[peak().value().value.value()]);
                    if(!m_super.empty()) {
                        str->contType = m_super.back();
                        m_super.back()->contained.push_back(str);
                    }

                    str->name = consume().value.value();
                    varHolder = str;
                    NodeStmtScope* scope = parseScope().value();
                    varHolder = nullptr;
                    for (auto stmt : scope->stmts) {
                        if(auto val = dynamic_cast<NodeStmtLet *>(stmt)) {
                            if(val->_static) {
                                if(auto type = dynamic_cast<NodeStmtStructNew*>(stmt))str->staticTypeName[type->mangle()] = type;
                                //else str->typeName nullptr;
                                str->staticVars.push_back(val);
                                str->staticTypes.push_back(val->type);
                            }else {
                                if(auto type = dynamic_cast<NodeStmtStructNew*>(stmt))str->typeName.push_back(type);
                                else str->typeName.emplace_back(nullptr);
                                str->vars.push_back(val);
                                str->types.push_back(val->type);
                            }
                        }else {
                            err("Only Fields/Attributes/Variables are allowed in structs");
                        }
                    }
                    return str;
                }
                case TokenType::_class: {
                    Class* clazz = dynamic_cast<Class *>(m_file->nameTypeMap[peak().value().value.value()]);
                    if(!m_super.empty()) {
                        clazz->contType = m_super.back();
                        m_super.back()->contained.push_back(clazz);
                    }

                    if(tryConsume(TokenType::extends)) {
                        if(auto cont = parseContained()) {
                            if(auto ext = dynamic_cast<Class*>(m_file->nameTypeMap[cont.value()->mangle()])) {
                                clazz->extending = ext;
                            }err("Can only extend classes");
                        }err("Expected class name after \"extending\"");
                    }

                    if(tryConsume(TokenType::implements)) {
                        bool next = true;
                        while (auto cont = parseContained()) {
                            if(!next)err("Expected comma between typenames");
                            if(auto imp = dynamic_cast<Interface*>(m_file->nameTypeMap[cont.value()->mangle()])){
                                clazz->implementing.push_back(imp);
                                next = tryConsume(TokenType::comma).has_value();
                            }err("Can only implement interfaces");
                        }
                        if(clazz->implementing.empty())err("Expected interface name after \"implements\"");
                    }

                    clazz->name = tryConsume(TokenType::id,"Expected identifier").value.value();
                    tryConsume(TokenType::openCurl,"Expected '{'");
                    m_super.push_back(clazz);

                    varTypes.emplace_back();
                    while (peak().has_value() && peak().value().type != TokenType::closeCurl) {
                        if(auto stmt = parseStmt()) {
                            if(auto var = dynamic_cast<NodeStmtLet*>(stmt.value())) {
                                if(auto type = dynamic_cast<NodeStmtStructNew*>(stmt.value()))clazz->typeName.push_back(type);
                                else clazz->typeName.emplace_back(nullptr);
                                clazz->vars.push_back(var);
                                clazz->types.push_back(var->type);
                            }else {
                                err("Only Fields/Attributes/Variables are allowed in structs");
                            }
                        }else if(auto func = parseFunc()) {
                            clazz->funcs.push_back(func.value());
                        }else if(auto type = parseType()) {
                            clazz->contained.push_back(type.value());
                        } else err("Expected Type or Variable");
                    }
                    varTypes.pop_back();
                    tryConsume(TokenType::closeCurl,"Expected '}'");
                    m_super.pop_back();
                    return clazz;
                }
                case TokenType::_namespace: {
                    NamesSpace* nmspc = dynamic_cast<NamesSpace *>(m_file->nameTypeMap[peak().value().value.value()]);
                    if(!m_super.empty()) {
                        nmspc->contType = m_super.back();
                        m_super.back()->contained.push_back(nmspc);
                    }
                    nmspc->name = tryConsume(TokenType::id,"Expected identifier").value.value();
                    tryConsume(TokenType::openCurl,"Expected '{'");
                    m_super.push_back(nmspc);

                    while (peak().has_value() && peak().value().type != TokenType::closeCurl) {
                            if(auto func = parseFunc()) {
                                nmspc->funcs.push_back(func.value());
                            }else if(auto type = parseType()) {
                                //nmspc->contained.push_back(type.value());
                            }else err("Expected type or function in namespace");
                    }
                    tryConsume(TokenType::closeCurl,"Expected '}'");
                    m_super.pop_back();
                    return nmspc;
                }
                default:
                    return {};
            }
        }

        SrcFile* Parser::parseProg(SrcFile* file){
            m_tokens = file->tokens;
            m_I = file->tokenPtr;
            m_file = file;
            while (peak().has_value()){
                if(auto func = parseFunc()) {
                    file->funcs.insert(std::pair(func.value()->mangle(),func.value()));
                }else if(auto type = parseType()){
                    file->types.push_back(type.value());
                }else {
                    err("Invalid Expression3");
                }
            }
            file->tokenPtr = m_I;
            return file;
        }