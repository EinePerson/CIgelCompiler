//
// Created by igel on 17.11.23.
//

#ifndef TYPES_H
#define TYPES_H
#include <variant>
#include <llvm/Support/FileSystem.h>
#include <llvm/IR/IRBuilder.h>

#include "tokenizer.h"
#include "exceptionns/Generator.h"

class CXX_Parser;
struct IgType;
struct FuncSig;
struct Class;
struct Var;
struct IgFunction;
struct ContainableType;

struct SrcFile;

namespace Igel {
    int getSize(TokenType type);

    enum class VarType {
        PirimVar,
        ArrayVar,
        StructVar,
        ClassVar,
    };

    struct Access {
#define PUBLIC_MASK 0b00000100
#define PROTECTED_MASK 0b00000010
#define PRIVATE_MASK 0b00000001
    private:
        char access = PUBLIC_MASK;
    public:
        [[nodiscard]] bool isPrivate() const {
            return access == PRIVATE_MASK;
        }

        [[nodiscard]] bool isProtected() const {
            return access == PROTECTED_MASK;
        }

        [[nodiscard]] bool isPublic() const {
            return access == PUBLIC_MASK;
        }

        void setPrivate() {
            access = PRIVATE_MASK;
        }

        void setProtected() {
            access = PROTECTED_MASK;
        }

        void setPublic() {
            access = PUBLIC_MASK;
        }
    };

    class SecurityManager {
    public:
        static bool canAccess(ContainableType* type,IgFunction* func);

        /**
         * \brief checks weather accessed can be accessed from accessor, access describes the governing access modifier
         * @param accesed the source object
         * @param accessor the object accessing
         * @param access the acces modifier used by the source object
         * @return wether this is possible
         */
        static bool canAccess(ContainableType* accesed,ContainableType* accessor,Access access);
    };
}

std::pair<llvm::FunctionCallee,bool> getFunction(std::string name,std::vector<llvm::Type *> types);
std::pair<llvm::FunctionCallee,bool> getFunction(std::string name,std::vector<llvm::Type*> types,std::vector<std::string> typeNames,std::vector<bool> signage);
llvm::Type* getType(TokenType type);

struct NodeStmtScope;
struct NodeStmt;
struct IgType;
/**
 * \brief every type extending this may have a super type
 */
struct BeContained {
    std::optional<BeContained*> contType = {};
    std::string name;

    bool mangleThis = true;

    BeContained() = default;
    virtual ~BeContained() = default;
    virtual std::string mangle() = 0;
};



struct FuncInfo {

};

llvm::Type* getType(llvm::Type* type);

llvm::Value* generateS(std::variant<NodeStmtScope*,NodeStmt*> var,llvm::IRBuilder<>* builder);

inline llvm::CmpInst::Predicate condition(const TokenType type,bool _signed) {
    if(_signed) {
        switch (type) {
            case TokenType::equal: return llvm::CmpInst::ICMP_EQ;
            case TokenType::notequal: return llvm::CmpInst::ICMP_NE;
            case TokenType::big: return llvm::CmpInst::ICMP_SGT;
            case TokenType::small: return llvm::CmpInst::ICMP_SLT;
            case TokenType::bigequal: return llvm::CmpInst::ICMP_SGE;
            case TokenType::smallequal: return llvm::CmpInst::ICMP_SLE;
            default: return llvm::CmpInst::BAD_ICMP_PREDICATE;
        }
    }
    switch (type) {
        case TokenType::equal: return llvm::CmpInst::ICMP_EQ;
        case TokenType::notequal: return llvm::CmpInst::ICMP_NE;
        case TokenType::big: return llvm::CmpInst::ICMP_UGT;
        case TokenType::small: return llvm::CmpInst::ICMP_ULT;
        case TokenType::bigequal: return llvm::CmpInst::ICMP_UGE;
        case TokenType::smallequal: return llvm::CmpInst::ICMP_ULE;
        default: return llvm::CmpInst::BAD_ICMP_PREDICATE;
    }
}

llvm::Value* generateReassing(llvm::Value* load,llvm::Value* _new,TokenType op,llvm::IRBuilder<>* builder);

struct Node {
    virtual ~Node() = default;

    Position pos;
    virtual llvm::Value* generate(llvm::IRBuilder<>* builder) = 0;

    virtual llvm::Value* generatePointer(llvm::IRBuilder<>* builder) = 0;
};

struct NodeExpr : Node {
    //FLOATING IS STORED IN VALUE* TYPE
    bool floating;
    bool _signed;
};
struct NodeStmt;
struct NodeTermFuncCall;



//BEGIN OF NODE TERMS

struct NodeTerm : NodeExpr{
    std::optional<NodeTerm*> contained;
    ContainableType* superType = nullptr;
};

struct NodeTermIntLit final : NodeTerm{
    Token int_lit;
    char sid = 2;
    llvm::Value* generate(llvm::IRBuilder<>* builder) override;

    llvm::Value* generatePointer(llvm::IRBuilder<>* builder) override {
        throw IllegalGenerationException("Cannot generate pointer to integer literal");
    };
};

struct NodeTermStringLit final : NodeTerm {
    Token str;
    llvm::Value* generate(llvm::IRBuilder<>* builder) override;
    llvm::Value* generatePointer(llvm::IRBuilder<>* builder) override {
        throw IllegalGenerationException("Cannot generate pointer to string literal");
    };
};

struct NodeTermNull final : NodeTerm {
    llvm::Value* generate(llvm::IRBuilder<>* builder) override {
        return llvm::ConstantPointerNull::get(llvm::PointerType::get(builder->getContext(),0));
    }

    llvm::Value* generatePointer(llvm::IRBuilder<>* builder) override {
        throw IllegalGenerationException("Cannot generate pointer to null");
    };
};

struct NodeTermId final : NodeTerm{
    BeContained* cont = nullptr;
    llvm::Value* generate(llvm::IRBuilder<>* builder) override;
    llvm::Value* generatePointer(llvm::IRBuilder<>* builder) override;
};

struct Name final : BeContained {
    Position pos;
    explicit Name(std::string name) {
        this->name = name;
    }
    NodeTermId* getId() {
        auto id = new NodeTermId;
        id->pos = pos;
        id->cont = this;
        return id;
    }
    std::string mangle() override;
};

struct NodeTermParen final : NodeTerm {
    NodeExpr* expr = nullptr;
    llvm::Value* generate(llvm::IRBuilder<>* builder) override {
        return expr->generate(builder);
    };
    llvm::Value* generatePointer(llvm::IRBuilder<>* builder) override {
        throw IllegalGenerationException("Cannot generate pointer to parenthese operation");
    };
};

struct NodeTermAcces : NodeTerm {
    NodeTermFuncCall* call = nullptr;
    BeContained* id = nullptr;
    Token acc;

    llvm::Value * generate(llvm::IRBuilder<> *builder) override;

    llvm::Value * generatePointer(llvm::IRBuilder<> *builder) override;

    llvm::Value* generateClassPointer(llvm::IRBuilder<> *builder);
};

struct NodeTermArrayAcces final : NodeTerm{
    BeContained* cont = nullptr;
    std::vector<NodeExpr*> exprs;
    llvm::Value* generate(llvm::IRBuilder<>* builder) override;
    llvm::Value* generatePointer(llvm::IRBuilder<>* builder) override;
};

struct NodeTermArrayLength final :NodeTerm {
    NodeTermArrayAcces* arr = nullptr;
    llvm::Value* generate(llvm::IRBuilder<>* builder) override;

    llvm::Value* generatePointer(llvm::IRBuilder<>* builder) override;
};

struct NodeTermStructAcces final : NodeTermAcces {
    llvm::Value* generate(llvm::IRBuilder<>* builder) override;
    llvm::Value* generatePointer(llvm::IRBuilder<>* builder) override;
};

struct NodeTermClassAcces final : NodeTermAcces {
    llvm::Value* generate(llvm::IRBuilder<>* builder) override;
    llvm::Value* generatePointer(llvm::IRBuilder<>* builder) override;
};

struct NodeTermFuncCall final : NodeTerm{
    BeContained* name = nullptr;
    std::vector<NodeExpr*> exprs;
    std::vector<llvm::Type*> params;
    std::vector<bool> signage;
    std::vector<BeContained*> typeNames;
    llvm::Value* generate(llvm::IRBuilder<>* builder) override;

    llvm::Value* generatePointer(llvm::IRBuilder<>* builder) override {
        throw IllegalGenerationException("Cannot generate pointer to function call");
    };
};

struct NodeTermClassNew final : NodeTerm {
    std::vector<NodeExpr*> exprs;
    BeContained* typeName;
    std::vector<bool> signage {};
    std::vector<llvm::Type*> paramType {};
    std::vector<BeContained*> paramTypeName {};
    llvm::Value* generate(llvm::IRBuilder<>* builder) override;

    llvm::Value* generatePointer(llvm::IRBuilder<>* builder) override {
        throw IllegalGenerationException("Cannot generate pointer to new instruction");
    };
};

struct NodeTermArrNew final : NodeTerm {
    char sid;
    bool _signed;
    std::vector<NodeExpr*> size;
    std::optional<BeContained*> typeName;
    llvm::Value* generate(llvm::IRBuilder<>* builder) override;

    llvm::Value* generatePointer(llvm::IRBuilder<>* builder) override {
        throw IllegalGenerationException("Cannot generate pointer to new array");
    };
};

struct NodeTermCast final : NodeTerm {
    BeContained* target = nullptr;
    NodeExpr* expr = nullptr;
    llvm::Value* generate(llvm::IRBuilder<>* builder) override;

    llvm::Value* generatePointer(llvm::IRBuilder<>* builder) override;
};

struct NodeTermInstanceOf final : NodeTerm {
    BeContained* target = nullptr;
    NodeExpr* expr = nullptr;
    llvm::Value* generate(llvm::IRBuilder<>* builder) override;

    llvm::Value* generatePointer(llvm::IRBuilder<>* builder) override;
};

//BEGIN OF BINARY EXPRESIONS

struct NodeBinExpr : NodeExpr{
    NodeExpr* ls = nullptr;
    NodeExpr* rs = nullptr;

    llvm::Value* generatePointer(llvm::IRBuilder<>* builder) override {
        throw IllegalGenerationException("Cannot generate pointer to binary expresion");
    };
};

struct NodeBinExprAdd final : NodeBinExpr{
    llvm::Value* generate(llvm::IRBuilder<>* builder) override {
        llvm::Value* lsv = ls->generate(builder);
        llvm::Value* rsv = rs->generate(builder);
        char type = 0;
        type += rsv->getType()->isFloatingPointTy()? 1 : 0;
        type += lsv->getType()->isFloatingPointTy()? 2 : 0;
        if(type == 0)return  builder->CreateAdd(lsv,rsv);

        if(type == 2)rsv = rs->_signed ? builder->CreateSIToFP(rsv,lsv->getType()) : builder->CreateUIToFP(rsv,lsv->getType());
        if(type == 1)lsv = ls->_signed ? builder->CreateSIToFP(lsv,rsv->getType()) : builder->CreateUIToFP(lsv,rsv->getType());

        return builder->CreateFAdd(lsv,rsv);
    };
};

struct NodeBinExprSub final : NodeBinExpr{
    llvm::Value* generate(llvm::IRBuilder<>* builder) override {
        llvm::Value* lsv = ls->generate(builder);
        llvm::Value* rsv = rs->generate(builder);
        char type = 0;
        type += rsv->getType()->isFloatingPointTy()? 1 : 0;
        type += lsv->getType()->isFloatingPointTy()? 2 : 0;
        if(type == 0)return  builder->CreateSub(lsv,rsv);

        if(type == 2)rsv = rs->_signed ? builder->CreateSIToFP(rsv,lsv->getType()) : builder->CreateUIToFP(rsv,lsv->getType());
        if(type == 1)lsv = ls->_signed ? builder->CreateSIToFP(lsv,rsv->getType()) : builder->CreateUIToFP(lsv,rsv->getType());

        return builder->CreateFSub(lsv,rsv);
    };
};

struct NodeBinExprMul final : NodeBinExpr{
    llvm::Value* generate(llvm::IRBuilder<>* builder) override {
        llvm::Value* lsv = ls->generate(builder);
        llvm::Value* rsv = rs->generate(builder);
        char type = 0;
        type += rsv->getType()->isFloatingPointTy()? 1 : 0;
        type += lsv->getType()->isFloatingPointTy()? 2 : 0;
        if(type == 0)return  builder->CreateMul(lsv,rsv);

        if(type == 2)rsv = rs->_signed ? builder->CreateSIToFP(rsv,lsv->getType()) : builder->CreateUIToFP(rsv,lsv->getType());
        if(type == 1)lsv = ls->_signed ? builder->CreateSIToFP(lsv,rsv->getType()) : builder->CreateUIToFP(lsv,rsv->getType());

        return builder->CreateFMul(lsv,rsv);
    };
};

struct NodeBinExprDiv final : NodeBinExpr{
    llvm::Value* generate(llvm::IRBuilder<>* builder) override {
        llvm::Value* lsv = ls->generate(builder);
        llvm::Value* rsv = rs->generate(builder);
        char type = 0;
        type += rsv->getType()->isFloatingPointTy()? 1 : 0;
        type += lsv->getType()->isFloatingPointTy()? 2 : 0;
        if(type == 0) {
            if(ls->_signed && rs->_signed)return builder->CreateSDiv(lsv,rsv);
            return builder->CreateUDiv(lsv,rsv);
        }

        if(type == 2)
            rsv = rs->_signed ? builder->CreateSIToFP(rsv,lsv->getType()) : builder->CreateUIToFP(rsv,lsv->getType());
        if(type == 1)
            lsv = ls->_signed ? builder->CreateSIToFP(lsv,rsv->getType()) : builder->CreateUIToFP(lsv,rsv->getType());

        return builder->CreateFDiv(lsv,rsv);
    };
};

struct NodeBinAreth final : NodeBinExpr{
    TokenType type = TokenType::uninit;
    llvm::Value* generate(llvm::IRBuilder<>* builder) override {
        llvm::AllocaInst* ptr = builder->CreateAlloca(llvm::Type::getInt1Ty(builder->getContext()));
        builder->CreateLoad(ptr->getType(),ptr);
        llvm::Value* lsv = ls->generate(builder);
        llvm::Value* rsv = rs->generate(builder);
        char typeI = 0;
        const llvm::CmpInst::Predicate lType = condition(type,ls->_signed || rs->_signed);
        typeI += rsv->getType()->isFloatingPointTy()? 1 : 0;
        typeI += lsv->getType()->isFloatingPointTy()? 2 : 0;
        if(typeI == 0) {
            return builder->CreateICmp(lType,lsv,rsv);
        }

        if(typeI == 2)
            rsv = rs->_signed ? builder->CreateSIToFP(rsv,lsv->getType()) : builder->CreateUIToFP(rsv,lsv->getType());
        if(typeI == 1)
            lsv = ls->_signed ? builder->CreateSIToFP(lsv,rsv->getType()) : builder->CreateUIToFP(lsv,rsv->getType());

        return builder->CreateFCmp(lType,lsv,rsv);
    };
};

struct NodeBinBit final : NodeBinExpr {
    TokenType op = TokenType::uninit;
    llvm::Value* generate(llvm::IRBuilder<>* builder) override {
        llvm::AllocaInst* ptr = builder->CreateAlloca(llvm::Type::getInt1Ty(builder->getContext()));
        builder->CreateLoad(ptr->getType(),ptr);
        llvm::Value* lsv = ls->generate(builder);
        llvm::Value* rsv = rs->generate(builder);
        switch (op) {
            case TokenType::_bitAnd:
                return builder->CreateAnd(lsv,rsv);
            case TokenType::_bitOr:
                return builder->CreateOr(lsv,rsv);
            default:
                return nullptr;
        }
    };
};

struct NodeBinNeg final : NodeBinExpr {
    NodeExpr* expr = nullptr;
    llvm::Value* generate(llvm::IRBuilder<>* builder) override {
        llvm::Value * val = expr->generate(builder);
        return builder->CreateNot(val);
    }
};

///\brief Technically a term but design requires it to be of type NodeBinExpr
struct NodeTermInlineIf final : NodeBinExpr {
    NodeExpr* cond = nullptr;
    llvm::Value* generate(llvm::IRBuilder<>* builder) override;
};

//BEGIN OF STATEMENT NODES

struct NodeStmt : Node {

    llvm::Value* generatePointer(llvm::IRBuilder<>* builder) override {
        throw IllegalGenerationException("Cannot generate pointer to statement");
    };
};

struct NodeStmtFuncCall final : NodeStmt{
    NodeTermFuncCall* call = nullptr;
    llvm::Value* generate(llvm::IRBuilder<>* builder) override {
        return call->generate(builder);
    };
};

struct NodeStmtSuperConstructorCall final : NodeStmt {
    std::vector<NodeExpr*> exprs {};
    Class* _this = nullptr;

    llvm::Value * generate(llvm::IRBuilder<> *builder) override;
};

struct NodeStmtLet : NodeStmt,BeContained{
    llvm::Type* type = nullptr;
    IgType* typeName = nullptr;
    bool final = false;
    bool _static = false;

    Igel::Access acc;
    std::pair<llvm::Value*, Var*> virtual generateImpl(llvm::IRBuilder<>* builder,bool full) = 0;;
    llvm::Value* generate(llvm::IRBuilder<>* builder) override;
    llvm::Value* generatePointer(llvm::IRBuilder<> *builder) override;
};

struct NodeStmtPirimitiv final : NodeStmtLet {
    char sid = -1;
    bool _signed = true;
    std::optional<NodeExpr*> expr = {};

    std::pair<llvm::Value*, Var*> generateImpl(llvm::IRBuilder<>* builder,bool full) override;

    std::string mangle() override;
};

struct NodeStmtNew : NodeStmtLet{
    std::vector<NodeExpr*> exprs = {};
    std::string mangle() override;
};

struct NodeStmtStructNew final : NodeStmtNew{
    std::pair<llvm::Value*, Var*> generateImpl(llvm::IRBuilder<>* builder,bool full) override;
};

struct NodeStmtClassNew final : NodeStmtNew {
    NodeTerm* term = nullptr;

    std::pair<llvm::Value*, Var*> generateImpl(llvm::IRBuilder<>* builder,bool full) override;
};

struct NodeStmtArr final : NodeStmtLet{
    NodeTerm* term = nullptr;
    char sid = -1;
    bool _signed = false;
    bool fixed = false;;
    uint size = 0;
    std::pair<llvm::Value*, Var*> generateImpl(llvm::IRBuilder<>* builder,bool full) override;

    std::string mangle() override;
};

struct NodeStmtEnum final : NodeStmtLet {
    uint id = -1;

    std::string mangle() override;

    std::pair<llvm::Value *, Var *> generateImpl(llvm::IRBuilder<> *builder,bool full) override;


};

struct NodeStmtExit final : NodeStmt{
    NodeExpr* expr = nullptr;
    llvm::Value* generate(llvm::IRBuilder<>* builder) override;
};

struct  NodeStmtScope final : NodeStmt{
    std::vector<NodeStmt*> stmts;
    int startIndex = 0;
    llvm::Value* generate(llvm::IRBuilder<>* builder) override;
};

struct NodeStmtIf final : NodeStmt{
    std::vector<NodeExpr*> expr;
    std::vector<TokenType> exprCond;
    std::variant<NodeStmtScope*,NodeStmt*> scope;
    std::optional<std::variant<NodeStmtScope*,NodeStmt*>> scope_else;
    std::optional<NodeStmtIf*> else_if;
    llvm::Value* generate(llvm::IRBuilder<>* builder) override;
};

struct NodeStmtReassign;

struct NodeStmtFor final : NodeStmt {
    NodeStmtLet* let = nullptr;
    std::vector<NodeExpr*> expr = {};
    std::vector<TokenType> exprCond = {};
    NodeStmtReassign* res = nullptr;
    std::variant<NodeStmtScope*,NodeStmt*> scope;
    llvm::Value* generate(llvm::IRBuilder<>* builder) override;
};

struct NodeStmtWhile final : NodeStmt {
    std::vector<NodeExpr*> expr = {};
    std::vector<TokenType> exprCond = {};
    std::variant<NodeStmtScope*,NodeStmt*> scope;
    llvm::Value* generate(llvm::IRBuilder<>* builder) override;
};

struct NodeStmtCase final : NodeStmt {
    NodeTermIntLit* cond = nullptr;
    bool _default = false;
    llvm::BasicBlock* generate(llvm::IRBuilder<>* builder) override;
};

struct NodeStmtSwitch final : NodeStmt {
    NodeStmtScope* scope {};
    NodeTerm* cond = nullptr;
    llvm::Value* generate(llvm::IRBuilder<>* builder) override;
};

struct NodeStmtReassign final : NodeStmt{
    NodeTerm* id = nullptr;
    NodeExpr* expr = nullptr;
    TokenType op = TokenType::uninit;
    llvm::Value* generate(llvm::IRBuilder<>* builder) override;
};

struct NodeStmtArrReassign final : NodeStmt{
    std::optional<NodeExpr*> expr;
    TokenType op = TokenType::uninit;
    NodeTermArrayAcces* acces = nullptr;
    llvm::Value* generate(llvm::IRBuilder<>* builder) override;
};

struct NodeStmtReturn final : NodeStmt{
    std::optional<NodeExpr*> val;
    llvm::Value* generate(llvm::IRBuilder<>* builder) override;
};

struct NodeStmtBreak final : NodeStmt {
    llvm::Value* generate(llvm::IRBuilder<>* builder) override;
};

struct NodeStmtContinue final : NodeStmt {
    llvm::Value* generate(llvm::IRBuilder<>* builder) override;
};

struct NodeStmtThrow final : NodeStmt{
    NodeExpr* expr = nullptr;
    llvm::Value* generate(llvm::IRBuilder<>* builder) override;
};

struct NodeStmtCatch final : NodeStmt{
    BeContained* typeName;
    std::string varName;
    NodeStmtScope* scope;
    llvm::Value* generate(llvm::IRBuilder<>* builder) override;
};

class FunctionCallee;

struct NodeStmtTry final : NodeStmt{
    static FunctionCallee* call;
    NodeStmtScope* scope {};
    std::vector<NodeStmtCatch*> catch_ {};
    llvm::Value* generate(llvm::IRBuilder<>* builder) override;
};

struct Class;

struct IgFunction final : BeContained{
    friend Class;
    std::vector<llvm::Type*> paramType;
    std::vector<std::string> paramName;
    std::vector<BeContained*> paramTypeName;
    std::vector<bool> signage;

    std::string retTypeName;
    llvm::Type* _return = nullptr;
    bool returnSigned = false;
    bool _static = false;
    bool final = false;
    bool member = false;
    bool destructor = false;
    bool constructor = false;
    NodeStmtScope* scope = nullptr;

    std::optional<Class*> supper {};
    bool abstract = false;
    bool _override = false;

    bool externC = false;
    Igel::Access acc;
    Position pos;
    std::pair<llvm::Function*,FuncSig*> genFuncSig(llvm::IRBuilder<>* builder);
    void genFunc(llvm::IRBuilder<>* builder);

    std::string mangle() override;

    llvm::Function* getLLVMFunc();
    llvm::FunctionType* getLLVMFuncType();

    void reset();

private:
    llvm::FunctionType* type = nullptr;
    llvm::Function* llvmFunc = nullptr;

    bool genSig = false;
    bool gen = false;
};

//BEGIN OF TYPES

struct IgType : BeContained {
    explicit IgType() = default;

    std::unordered_map<std::string,Igel::VarType> varTypes = {};
    std::unordered_map<std::string,std::string> varTypeNames = {};

    llvm::DICompositeType* dbgType = nullptr;
    llvm::DIDerivedType* dbgpointerType = nullptr;

    bool _extern = false;

    virtual void generateSig(llvm::IRBuilder<>* builder) = 0;
    virtual void generatePart(llvm::IRBuilder<>* builder) = 0;
    virtual void generate(llvm::IRBuilder<>* builder) = 0;
    virtual void unregister() = 0;

};

struct Struct final : IgType {
    Struct() : IgType(){}
    std::vector<NodeStmtLet*> vars;
    std::vector<NodeStmtLet*> staticVars;
    std::vector<std::string> varIds;
    std::vector<llvm::Type*> types {};
    std::vector<llvm::Type*> staticTypes {};
    std::vector<BeContained*> typeName;
    std::unordered_map<std::string,BeContained*> staticTypeName;
    std::unordered_map<std::string,uint> varIdMs;
    std::unordered_map<std::string,bool> finals;
    std::vector<bool> signage;


    Position pos;

    void generateSig(llvm::IRBuilder<>* builder) override;

    void generatePart(llvm::IRBuilder<> *builder) override;

    void generate(llvm::IRBuilder<>* builder) override;

    std::string mangle() override;

    void unregister() override {
        strType = nullptr;
    }

    llvm::StructType* getStrType(llvm::IRBuilder<> *builder);

private:
    llvm::StructType* strType = nullptr;

};

/**
 * \brief A type that can contain different types;
 */
struct ContainableType : IgType {
    explicit ContainableType() = default;
    std::vector<BeContained*> contained {};

    /**
     * @param type
     * @return checks weather type is a super of this
     */
    virtual bool isSubTypeOf(ContainableType* type) = 0;

    /**
     *\brief returns -1 if not found
     */
    virtual uint getSuperOffset(ContainableType* type) = 0;

    virtual uint getExtendingOffset() = 0;

    virtual std::vector<std::vector<IgFunction*>> getFuncsRec() = 0;

    virtual std::vector<llvm::Constant*> getTypeInfoRec(llvm::IRBuilder<> *builder) = 0;
};

struct NamesSpace final : ContainableType {
    NamesSpace() : ContainableType(){}
    std::vector<IgFunction*> funcs {};

    void generateSig(llvm::IRBuilder<>* builder) override;

    void generatePart(llvm::IRBuilder<> *builder) override {

    };

    void generate(llvm::IRBuilder<>* builder) override;

    std::string mangle() override;

    std::vector<std::vector<IgFunction*>> getFuncsRec() override {
        return {funcs};
    }

    std::vector<llvm::Constant *> getTypeInfoRec(llvm::IRBuilder<> *builder) override {
        return  {};
    }

    void unregister() override{}

    bool isSubTypeOf(ContainableType* type) override {
        return false;
    }

    uint getSuperOffset(ContainableType *type) override {
        return -1;
    }

    uint getExtendingOffset() override {
        return 0;
    }
};

struct Interface;

struct ClassInfos {
    friend Class;
    friend Interface;
    llvm::GlobalVariable* typeNameVar = nullptr;
    llvm::GlobalVariable* typeInfo = nullptr;
    llvm::GlobalVariable* vtable = nullptr;

    llvm::GlobalVariable* typeNamePointVar = nullptr;
    llvm::GlobalVariable* typeInfoPointVar = nullptr;

private:
    bool init = false;
};

struct Interface final : ContainableType {
    Interface() : ContainableType(){}

    std::vector<IgFunction*> funcs;
    std::vector<Interface*> extending {};
    bool gen = false;

    std::string mangle() override;

    void generateSig(llvm::IRBuilder<>* builder) override;

    void generatePart(llvm::IRBuilder<> *builder) override {

    };

    void generate(llvm::IRBuilder<>* builder) override;

    ClassInfos getClassInfos(llvm::IRBuilder<> *builder);

    std::vector<std::vector<IgFunction*>> getFuncsRec() override {
        std::vector<std::vector<IgFunction*>> funcsRet {};
        if(!funcs.empty()) funcsRet.push_back(funcs);

        for(auto& e : extending){
            auto extFuncs = e->getFuncsRec();
            funcsRet.insert(funcsRet.end(),extFuncs.begin(),extFuncs.end());
        }
        return funcsRet;
    }

    std::vector<llvm::Constant *> getTypeInfoRec(llvm::IRBuilder<> *builder) override {
        std::vector<llvm::Constant*> infoRec {};
        infoRec.reserve(extending.size());
        for (auto ext : extending)infoRec.push_back(ext->getClassInfos(builder).typeInfo);

        for(auto& e : extending){
            auto extFuncs = e->getTypeInfoRec(builder);
            infoRec.insert(infoRec.end(),extFuncs.begin(),extFuncs.end());
        }
        return infoRec;
    }

    void unregister() override {
        classInfos.init = false;
    }

    bool isSubTypeOf(ContainableType *type) override {
        if(std::count(extending.begin(),extending.end(),type)) return true;
        for (auto interface : extending) {
            if(interface->isSubTypeOf(type))return true;
        }
        return false;
    }

    uint getSuperOffset(ContainableType *type) override {
        if(!isSubTypeOf(type))return -1;
        if(std::count(extending.begin(),extending.end(),type)) return 0;
        uint offset = 0;
        for (auto interface : extending) {
            if(interface == type || interface->isSubTypeOf(type))return offset;
            offset += interface->getExtendingOffset();
        }
        return -1;

    }

    uint getExtendingOffset() override {
        return 8;
    }

private:
    ClassInfos classInfos;
};

struct Enum final : IgType {
    std::vector<std::string> values;
    std::unordered_map<std::string,uint> valueIdMap;

    std::string mangle() override;

    void generateSig(llvm::IRBuilder<> *builder) override;

    void generatePart(llvm::IRBuilder<> *builder) override;

    void generate(llvm::IRBuilder<> *builder) override;

    void unregister() override {}

};



struct Class final : ContainableType {
    Class() : ContainableType(){}
    std::vector<NodeStmtLet*> vars;
    std::vector<std::string> varNames;
    std::unordered_map<std::string,uint> varIdMs;
    std::unordered_map<std::string,Igel::Access> varAccesses;
    std::unordered_map<std::string,bool> finals;

    std::vector<NodeStmtLet*> staticVars;
    std::unordered_map<std::string,BeContained*> staticTypeName;

    std::vector<llvm::Type*> types {};
    std::vector<llvm::Type*> staticTypes {};
    std::vector<BeContained*> typeName;

    llvm::StructType* strType = nullptr;

    uint vTablePtrs = 0;
    std::optional<Class*> extending = {};
    std::vector<Interface*> implementing {};
    bool final = false;

    std::vector<IgFunction*> funcs;
    std::unordered_map<uint,std::unordered_map<uint,IgFunction*>> funcMap;
    std::unordered_map<std::string,std::pair<uint,uint>> funcIdMs;
    ///\brief c++ non-virtual functions
    std::unordered_map<std::string,IgFunction*> directFuncs;

    std::vector<IgFunction*> constructors {};
    std::optional<IgFunction*> defaulfConstructor = {};
    bool hasConstructor = false;

    Position pos;


    bool abstract = false;

    ///\brief generateSig was called
    bool wasGen = false;
    ///\brief generatePart was called
    bool partGen = false;
    ///\brief generate was called
    bool fullGen = false;

    void generateSig(llvm::IRBuilder<>* builder) override;

    void generatePart(llvm::IRBuilder<> *builder) override;

    void generate(llvm::IRBuilder<>* builder) override;

    std::string mangle() override;

    ClassInfos getClassInfos(llvm::IRBuilder<> *builder);

    std::pair<Class*,uint> getVarialbe(const std::string &name) {
        if(varIdMs.contains(name))return {this,varIdMs[name]};
        else {
            if(!extending.has_value())return {nullptr,0};
            return extending.value()->getVarialbe(name);
        }
    }

    std::optional<IgFunction*> getFunction(const std::string &name) {
        if(funcIdMs.contains(name))return funcMap[funcIdMs[name].first][funcIdMs[name].second];
        else {
            if(!extending.has_value())return {};
            return extending.value()->getFunction(name);
        }

    }

    std::vector<std::vector<IgFunction *>> getFuncsRec() override {
        std::vector<std::vector<IgFunction *>> funcsRet {funcs};
        if(extending.has_value()) {
            auto extFuncs = extending.value()->getFuncsRec();
            funcsRet[0].insert(funcsRet[0].begin(),extFuncs[0].begin(),extFuncs[0].end());
            extFuncs.erase(extFuncs.begin());
            funcsRet.insert(funcsRet.end(),extFuncs.begin(),extFuncs.end());
        }

        for(const auto& e : implementing){
            auto funccpy = e->getFuncsRec();
            funcsRet.reserve(funccpy.size());
            for(auto& f : funccpy){
                funcsRet.push_back(f);
            }
        }
        return funcsRet;
    }

    std::vector<llvm::Constant*> getTypeInfoRec(llvm::IRBuilder<> *builder) override {
        std::vector<llvm::Constant*> infoRec {};
        infoRec.reserve(implementing.size());
        for (auto imp : implementing)infoRec.push_back(imp->getClassInfos(builder).typeInfo);

        if(extending.has_value()) {
            infoRec.push_back(extending.value()->getClassInfos(builder).typeInfo);
            auto extFuncs = extending.value()->getTypeInfoRec(builder);
            infoRec.insert(infoRec.end(),extFuncs.begin(),extFuncs.end());
        }

        for(auto& e : implementing){
            auto extFuncs = e->getTypeInfoRec(builder);
            infoRec.insert(infoRec.end(),extFuncs.begin(),extFuncs.end());
        }
        return infoRec;
    }

    void unregister() override {
        classInfos.init = false;
        for (auto func : funcs) func->reset();
    }

    bool isSubTypeOf(ContainableType *type) override {
        if(type == this)return true;
        if(std::count(implementing.begin(),implementing.end(),type)) return true;
        if(extending.has_value() && (extending.value() == type || extending.value()->isSubTypeOf(type)))return true;
        for (auto interface : implementing) {
            if(interface->isSubTypeOf(type))return true;
        }
        return false;
    }

    uint getExtendingOffset() override;

    uint getSuperOffset(ContainableType *type) override;

private:

    ClassInfos classInfos;
};

namespace Igel {
    inline void errAt(const Position &t) {
        std::cerr << "\n at: " << t.file << ":" << t.line << ":" << t._char << std::endl;
        exit(EXIT_FAILURE);
    }

    inline void err(const std::string &msg,const Position &t) {
        std::cerr << msg << std::endl;
        errAt(t);
    }

    inline void internalErr(const std::string &msg) {
        std::cerr << "Internal error: " << msg << std::endl;
        exit(EXIT_FAILURE);
    }

    inline void generalErr(const std::string &msg) {
        std::cerr << msg << std::endl;
        exit(EXIT_FAILURE);

    }

    ///\brief Returns the next size possible to store int bits
    inline int nextValidVarSize(int bits) {
        if(bits <= 1)return 1;
        if(bits <= 8)return 8;
        if(bits <= 16)return 16;
        if(bits <= 32)return 32;
        if(bits <= 64)return 64;
        return 128;
    }

    inline llvm::Type* nextValiedIntType(int bits,llvm::IRBuilder<> *builder) {
        if(bits <= 1)return builder->getInt1Ty();
        if(bits <= 8)return builder->getInt8Ty();
        if(bits <= 16)return builder->getInt16Ty();
        if(bits <= 32)return builder->getInt32Ty();
        if(bits <= 64)return builder->getInt64Ty();
        return builder->getInt128Ty();
    }

    inline llvm::Type* getTypeOfInt(int bits,llvm::IRBuilder<> *builder) {
        switch (bits) {
            case 1: return builder->getInt1Ty();
            case 8: return builder->getInt8Ty();
            case 16: return builder->getInt16Ty();
            case 32: return builder->getInt32Ty();
            case 64: return builder->getInt64Ty();
            case 128: return builder->getInt128Ty();
            default: return nullptr;
        }
    }

    llvm::Value* dyn_Cast(llvm::IRBuilder<> *builder,BeContained* target,BeContained* src,llvm::Value* val);

    llvm::Value* stat_Cast(llvm::IRBuilder<> *builder,BeContained* target,BeContained* src,llvm::Value* val);

    llvm::Instruction* setDbg(llvm::IRBuilder<> *builder,llvm::Instruction* inst,Position pos);
}

#endif //TYPES_H
