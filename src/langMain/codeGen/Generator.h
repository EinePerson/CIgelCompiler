//
// Created by igel on 16.11.23.
//

#ifndef IGEL_COMPILER_MAINGEN_H
#define IGEL_COMPILER_MAINGEN_H

#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include <memory>
#include <vector>
#include <llvm/Target/TargetMachine.h>

#include "../../CompilerInfo/InfoParser.h"
#include "../../types.h"

using namespace llvm;

struct SrcFile;
struct IgFunction;
struct BeContained;
struct Struct;

struct Var {
    explicit Var(AllocaInst* alloc,bool _signed) : alloc(alloc),_signed(_signed) {}
    virtual ~Var() = default;
    Value* alloc = nullptr;
    char _signed = -1;

    Type* getType() {
        Type* ty;
        if(llvm::AllocaInst::classof(alloc))ty = static_cast<AllocaInst*>(alloc)->getAllocatedType();
        else if(GlobalVariable::classof(alloc))ty = static_cast<GlobalVariable*>(alloc)->getValueType();
        return ty;
    }
};

struct ArrayVar : Var {
    explicit ArrayVar(AllocaInst* alloc,bool _signed,std::optional<BeContained*> typeName = {}) : Var(alloc,_signed),typeName(typeName) {}
    Type* type = nullptr;
    std::optional<BeContained*> typeName;
    uint size = -1;
};

struct StructVar final : Var {
    explicit StructVar(AllocaInst* alloc) : Var(alloc,false) {}
    std::unordered_map<std::string,uint> vars;
    std::vector<Type*> types;
    std::vector<bool> signage;
    StructType* strType = nullptr;
    Struct* str = nullptr;
};

struct ClassVar final : Var {
    explicit ClassVar(AllocaInst* alloc) : Var(alloc,false) {}
    std::unordered_map<std::string,uint> vars;
    std::vector<Type*> types;
    std::vector<bool> signage;

    std::unordered_map<std::string,uint> funcs;
    PointerType* type = nullptr;
    StructType* strType = nullptr;
    Class* clazz = nullptr;
};

class Generator {

public:
    explicit Generator(SrcFile* file,Info* info);
    Generator();
    ~Generator();
    void setup(SrcFile* file);

    void generate();

    [[nodiscard]] static Type* getType(TokenType type);

    std::pair<Function*,FuncSig*> genFuncSig(IgFunction* func);

    void genFunc(IgFunction* func,bool member = false);

    void reset(SrcFile* file);

    void write();

    llvm::Value* genStructVar(std::string typeName);

    Value* exitG(Value* exitCode) const;
    [[nodiscard]] Value* _new() const;

    Var* getVar(const std::string&name, bool _this = false);
    std::optional<Var*> getOptVar(std::string name, bool _this = false);

    void createVar(const std::string&name,Type* type,Value* val,bool _signed);
    void createVar(std::string name,Var* var);
    void createVar(Argument* arg,bool _signed, const std::string&typeName);

    void createStaticVar(std::string name,Value* val,Var* var);
private:
    BasicBlock* staticInit = nullptr;
    std::unique_ptr<IRBuilder<>> m_builder;
    bool setupFlag = false;
    std::string m_target_triple;
    DataLayout* m_layout;
    TargetMachine* m_machine;
    uint m_currentVar = 0;
    std::vector<uint> m_sVarId;
public:
    SrcFile* m_file;
    std::unique_ptr<Module> m_module;
    std::vector<std::map<std::string, Var*>> m_vars;
    std::vector<BasicBlock*> after;
    std::vector<BasicBlock*> next;
    std::vector<SwitchInst*> _switch;
    Info* m_info;
    static Generator* instance;
    static std::unique_ptr<LLVMContext> m_contxt;
    static std::vector<bool> unreachableFlag;
    static bool lastUnreachable;;
    static Struct* structRet;
    static Class* classRet;
    static bool arrRet;
    static BeContained* typeNameRet;
};


#endif //IGEL_COMPILER_MAINGEN_H
