//
// Created by igel on 12.01.24.
//

#include "Parser.h"

#include <llvm/Support/CommandLine.h>

std::optional<NodeTerm *> Parser::parseTerm() {
            NodeTerm* term;
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
                            err("Expected Expression");
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
                }else if(peak(1).has_value() && peak(1).value().type == TokenType::openBracket) {
                    auto* arr = new NodeTermArrNew;
                    arr->sid = -1;
                    arr->typeName = consume().value.value();
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
                }else{
                    auto cr = new NodeTermNew;
                    cr->typeName = tryConsume(TokenType::id,"Expected Identifier").value.value();
                    return cr;
                }
            } else if(tryConsume(TokenType::null)) {
                return new NodeTermNull;
            }else
                return {};
            if(tryConsume(TokenType::connector)) {
                auto termT = new NodeTermStructAcces;
                if(auto val = dynamic_cast<NodeTermId*>(term))termT->id = val->id;
                if(auto val = dynamic_cast<NodeTermFuncCall*>(term)) termT->call = val;
                if(auto val = dynamic_cast<NodeTermArrayAcces*>(term))termT->contained = term;

                auto termF = peak();
                if(peak(1).has_value() && peak(1).value().type != TokenType::connector && peak(1).value().type != TokenType::openBracket)consume();
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
            while(auto stmt = parseStmt()){
                scope->stmts.push_back(stmt.value());
            }
            tryConsume(TokenType::closeCurl,"Expected '}'");
            return scope;
        }

        std::optional<NodeStmt*> Parser::parseStmt(bool semi){
            if(peak().has_value() && peak().value().type <= TokenType::_bool){
                if(peak(1).has_value() && peak(1).value().type == TokenType::id && peak(2).has_value()
                && (peak(2).value().type == TokenType::eq || peak(2).value().type == TokenType::semi)) {
                    auto pirim = new NodeStmtPirimitiv;
                    pirim->type = getType(peak().value().type);
                    pirim->sid = (char) peak().value().type;
                    m_sidFlag = pirim->sid;
                    pirim->_signed = consume().type <= TokenType::_long;
                    pirim->name = consume().value.value();
                    if(!peak().has_value()){
                        err("Expected expression or '='");
                    }
                    if(peak().value().type == TokenType::semi){
                        consume();
                        auto intLit = new NodeTermIntLit;
                        Token tok{.type = TokenType::int_lit,.value = "0"};
                        intLit->_signed = false;
                        intLit->int_lit = tok;
                        pirim->expr = intLit;
                    } else {
                        tryConsume(TokenType::eq,"Expected '='");
                        if (auto expr = parseExpr())
                            pirim->expr = expr.value();
                        else {
                            err("Unexpected expression1");
                        }
                        tryConsume(TokenType::semi, "Expected ';'");
                    }
                    return pirim;
                }else if(peak(1).has_value() && peak(1).value().type == TokenType::openBracket){
                    auto arr = new NodeStmtArr;
                    arr->sid = (char) peak().value().type;
                    arr->_signed = consume().type <= TokenType::_long;
                    arr->type = reinterpret_cast<Type *>(1);
                    /*if(peak(1).has_value() && peak(1).value().type == TokenType::int_lit){
                        while (peak().has_value() && peak().value().type == TokenType::openBracket && peak(1).has_value() && peak(1).value().type == TokenType::int_lit &&
                        peak(2).has_value() && peak(2).value().type == TokenType::closeBracket){
                            consume();
                            //arr->size.push_back(stoi(tryConsume(TokenType::int_lit,"Expected Integer as value").value.value()));
                            consume();
                        }
                        arr->fixed = true;
                    }else{*/
                        while (peak().has_value() && peak().value().type == TokenType::openBracket && peak(1).has_value() && peak(1).value().type == TokenType::closeBracket){
                            consume();
                            //if(auto term = parseTerm())arr->size.push_back(term.value());
                            //else err("Expected Term");
                            arr->size++;
                            consume();
                        }
                        arr->fixed = false;
                    //}

                    arr->name = consume().value.value();
                    if(!arr->fixed && tryConsume(TokenType::eq)) {
                        if(auto t = parseTerm()) {
                            arr->term = t.value();
                        }else err("Expected Term");
                    }
                    tryConsume(TokenType::semi, "Expected ';'");
                    return arr;
                }
            }else if(auto scope = parseScope()){
                return scope.value();
            }else if(auto _if = tryConsume(TokenType::_if)){
                return parseIf();
            }else if(auto _for = tryConsume(TokenType::_for)) {
                return parseFor();
            }else if(auto _while = tryConsume(TokenType::_while)) {
                return parseWhile();
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
            }else if(peak().has_value() && peak().value().type == TokenType::id && std::find(m_file->typeNames.begin(),m_file->typeNames.end(),peak().value().value.value()) != m_file->typeNames.end()) {
                /*std::string typeName = consume().value.value();
                while (tryConsume(TokenType::dConnect)) {
                    tryConsume(TokenType::dConnect,"Expected ':'");
                    typeName += "::" + tryConsume(TokenType::id,"Expected id").value.value();
                }*/
                BeContained* cont = nullptr;
                if(auto contO = parseContained())cont = contO.value();
                else return {};

                if(peak().value().type == TokenType::openBracket) {
                    auto* arr = new NodeStmtArr;

                    arr->name = cont->name;
                    if(cont->contType.has_value())arr->contType = cont->contType.value();

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
                    return arr;
                }
                Token varName = tryConsume(TokenType::id,"Expected name");
                if(peak().value().type == TokenType::eq) {
                    auto strNew = new NodeStmtStructNew;
                    strNew->type = reinterpret_cast<Type *>(2);
                    strNew->name = cont->name;
                    if(cont->contType.has_value())strNew->contType = cont->contType.value();
                    strNew->name = varName.value.value();
                    tryConsume(TokenType::eq,"Expected '='");
                    if(auto t = parseTerm())
                        strNew->term = t.value();
                    else {
                        err("Expected Term after '='");
                    }
                    tryConsume(TokenType::semi,"Expected ';'");
                    return strNew;
                }else if(peak().value().type == TokenType::semi) {
                    auto strNew = new NodeStmtStructNew;
                    strNew->type = reinterpret_cast<Type *>(2);
                    strNew->name = cont->name;
                    if(cont->contType.has_value())strNew->contType = cont->contType.value();
                    strNew->name = varName.value.value();
                    strNew->term = new NodeTermNull;
                    consume();
                    return strNew;
                }
            }else /*if(peak().has_value() && peak().value().type == TokenType::id && peak(1).has_value() && peak(1).value().type == TokenType::id &&
                peak(2).has_value() && (peak(2).value().type == TokenType::eq || peak(2).value().type == TokenType::semi)/*&& peak(3).has_value() && peak(3).value().type == TokenType::_new) {

            } else*/ if(auto id = /*tryConsume(TokenType::id)*/ parseTerm()){
                if(auto val = dynamic_cast<NodeTermFuncCall*>(id.value())){
                    auto* call = new NodeStmtFuncCall;
                    call->call = val;
                    tryConsume(TokenType::semi, "Expected ';'");
                    return call;
                }else if(auto value = dynamic_cast<NodeTermArrayAcces *>(id.value())){
                    auto reassign = new NodeStmtArrReassign;
                    reassign->acces = value;
                    if (peak().has_value() && peak().value().type >= TokenType::eq &&
                            peak().value().type <= TokenType::pow_eq) {
                        reassign->op = consume().type;

                        if (auto expr = parseExpr())reassign->expr = expr.value();
                        else {
                            err("Unexpected expression2");
                        }
                    } else if (peak().has_value() && peak().value().type == TokenType::inc ||
                                       peak().value().type == TokenType::dec) {
                        reassign->op = consume().type;
                    } else {
                        err("Expected Assignment Operator1");
                    }
                    tryConsume(TokenType::semi, "Expected ';'");
                    return reassign;
                }else if(peak().has_value() && (peak().value().type == TokenType::inc || peak().value().type == TokenType::dec)){
                    auto reassign = new NodeStmtReassign;
                    reassign->op = consume().type;
                    reassign->id = id.value();

                    if(semi)tryConsume(TokenType::semi,"Expected ';'");
                    return  reassign;
                }else{
                        auto reassign = new NodeStmtReassign;
                        reassign->id = id.value();
                        if (peak().has_value() && peak().value().type >= TokenType::eq &&
                            peak().value().type <= TokenType::pow_eq) {
                            reassign->op = consume().type;

                            if (auto expr = parseExpr())reassign->expr = expr.value();
                            else {
                                err("Unexpected expression2");
                            }
                        } else if (peak().has_value() && peak().value().type == TokenType::inc ||
                                   peak().value().type == TokenType::dec) {
                            reassign->op = consume().type;
                        } else {
                            err("Expected Assignment Operator2");
                        }
                        if(semi)tryConsume(TokenType::semi, "Expected ';'");
                        return reassign;
                }
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
            if(!m_super.empty()) {
                func->contType = m_super.back();
                m_super.back()->contained.push_back(func);
            }
            if(peak().has_value() && (peak().value().type <= TokenType::_ulong || peak().value().type == TokenType::_void || peak().value().type == TokenType::id)) {
                Token ret = consume();
                if(ret.value.has_value())func->retTypeName = ret.value.value();
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
            while (peak().has_value() && (peak().value().type <= TokenType::_bool || peak().value().type == TokenType::id))
            {
                if(i)tryConsume(TokenType::comma,"Expected ','");
                func->signage.push_back(peak().value().type <= TokenType::_long);
                Token param = consume();
                if(param.value.has_value())func->paramTypeName.push_back(param.value.value());
                else func->paramTypeName.emplace_back("");
                func->paramType.push_back(getType(param.type));
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
    //TODO code
}

std::optional<IgType*> Parser::parseType() {
            if(peak(1).value().type != TokenType::id) return  {};
            switch (peak().value().type) {
                case TokenType::_struct: {
                    auto str = new Struct;
                    if(!m_super.empty()) {
                        str->contType = m_super.back();
                        m_super.back()->contained.push_back(str);
                    }
                    consume();
                    str->name = consume().value.value();
                    NodeStmtScope* scope = parseScope().value();
                    for (auto stmt : scope->stmts) {
                        if(auto val = dynamic_cast<NodeStmtLet *>(stmt)) {
                            if(auto type = dynamic_cast<NodeStmtStructNew*>(stmt))str->typeName.push_back(type);
                            else str->typeName.emplace_back(nullptr);
                            str->vars.push_back(val);
                            str->types.push_back(val->type);
                        }else {
                            err("Only Fields/Attributes/Variables are allowed in structs");
                        }
                    }
                    return str;
                }
                case TokenType::_class: {
                    auto clazz = new Class;
                    consume();
                    if(!m_super.empty()) {
                        clazz->contType = m_super.back();
                        m_super.back()->contained.push_back(clazz);
                    }
                    //TODO extends parsing
                    clazz->name = tryConsume(TokenType::id,"Expected identifier").value.value();
                    tryConsume(TokenType::openCurl,"Expected '{'");
                    m_super.push_back(clazz);

                    while (peak().has_value() && peak().value().type != TokenType::closeCurl) {
                        if(auto func = parseFunc()) {
                            clazz->funcs.push_back(func.value());
                        }else if(auto stmt = parseStmt()) {
                            if(auto var = dynamic_cast<NodeStmtLet*>(stmt.value())) {
                                if(auto type = dynamic_cast<NodeStmtStructNew*>(stmt.value()))clazz->typeName.push_back(type);
                                else clazz->typeName.emplace_back(nullptr);
                                clazz->vars.push_back(var);
                                clazz->types.push_back(var->type);
                            }else {
                                err("Only Fields/Attributes/Variables are allowed in structs");
                            }
                        }else if(auto type = parseType()) {
                            clazz->contained.push_back(type.value());
                        } else err("Expected Type or Variable");
                    }

                    tryConsume(TokenType::closeCurl,"Expected '}'");
                    m_super.pop_back();
                    return clazz;
                }
                case TokenType::_namespace: {
                    auto nmspc = new NamesSpace;
                    if(!m_super.empty()) {
                        nmspc->contType = m_super.back();
                        m_super.back()->contained.push_back(nmspc);
                    }
                    consume();
                    nmspc->name = tryConsume(TokenType::id,"Expected identifier").value.value();
                    tryConsume(TokenType::openCurl,"Expected '{'");
                    m_super.push_back(nmspc);

                    while (peak().has_value() && peak().value().type != TokenType::closeCurl) {
                            if(auto func = parseFunc()) {
                                nmspc->funcs.push_back(func.value());
                            }else if(auto type = parseType()) {
                                nmspc->contained.push_back(type.value());
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
                    file->funcs.insert(std::pair(FuncSig(func.value()->name,func.value()->paramType),func.value()));
                }else if(auto type = parseType()){
                    if(type.value()->gen)file->types.push_back(type.value());
                }else {
                    err("Invalid Expression3");
                }
            }
            file->tokenPtr = m_I;
            return file;
        }