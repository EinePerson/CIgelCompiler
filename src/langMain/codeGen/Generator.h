//
// Created by igel on 16.11.23.
//

#ifndef IGEL_COMPILER_MAINGEN_H
#define IGEL_COMPILER_MAINGEN_H

#include "llvm/IR/DIBuilder.h"
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
    explicit Var(AllocaInst* alloc,bool _signed,bool _final) : alloc(alloc),_signed(_signed),_final(_final) {}
    virtual ~Var() = default;
    Value* alloc = nullptr;
    char _signed = -1;
    bool _final;

    Type* getType() {
        Type* ty;
        if(llvm::AllocaInst::classof(alloc))ty = static_cast<AllocaInst*>(alloc)->getAllocatedType();
        else if(GlobalVariable::classof(alloc))ty = static_cast<GlobalVariable*>(alloc)->getValueType();
        return ty;
    }
};

struct ArrayVar : Var {
    explicit ArrayVar(AllocaInst* alloc,bool _signed,bool _final,std::optional<BeContained*> typeName = {}) : Var(alloc,_signed,_final),typeName(typeName) {}
    Type* type = nullptr;
    std::optional<BeContained*> typeName;
    uint size = -1;
};

struct StructVar final : Var {
    explicit StructVar(AllocaInst* alloc,bool _final) : Var(alloc,false,_final) {}
    std::unordered_map<std::string,uint> vars;
    std::vector<Type*> types;
    std::vector<bool> signage;
    StructType* strType = nullptr;
    Struct* str = nullptr;
};

struct ClassVar final : Var {
    explicit ClassVar(AllocaInst* alloc,bool _final) : Var(alloc,false,_final) {}
    std::unordered_map<std::string,uint> vars;
    std::vector<Type*> types;
    std::vector<bool> signage;

    std::unordered_map<std::string,uint> funcs;
    PointerType* type = nullptr;
    StructType* strType = nullptr;
    ContainableType* clazz = nullptr;
};

struct Debug {
    DIBuilder* builder;
    DICompileUnit* unit;
    DIFile* file;
};

class Generator {

public:
    explicit Generator(SrcFile* file,Info* info);
    Generator();
    ~Generator();
    void create(SrcFile* file);
    void setup(SrcFile* file);

    void generateSigs();

    void generate();

    [[nodiscard]] static Type* getType(TokenType type);

    void reset(SrcFile* file);

    void write();

    llvm::Value* genStructVar(std::string typeName);

    Value* exitG(Value* exitCode) const;
    [[nodiscard]] Value* _new() const;

    Var* getVar(const std::string&name, bool _this = false);
    std::optional<Var*> getOptVar(std::string name, bool _this = false);

    void createVar(const std::string&name,Type* type,Value* val,bool _signed,bool _final);
    void createVar(std::string name,Var* var);
    void createVar(Argument* arg,bool _signed, const std::string&typeName,bool createDbg = true);

    void createStaticVar(std::string name,Value* val,Var* var);

    void initInfo();

    void reset();

    Debug getDebug();
    DIScope* getScope() const {
        return dbgScopes.empty()?dbg.file:dbgScopes.back();
    }

    static unsigned getEncodingOfType(Type* type);
private:
    BasicBlock* staticInit = nullptr;
    std::unique_ptr<IRBuilder<>> m_builder;
    bool setupFlag = false;
    std::string m_target_triple;
    DataLayout* m_layout;
    TargetMachine* m_machine;
    uint m_currentVar = 0;
    std::vector<uint> m_sVarId;
    std::unordered_map<SrcFile*,Module*> modules;
    std::unordered_map<SrcFile*,std::pair<DICompileUnit*,DIFile*>> dbgModules;
    Debug dbg;
public:
    const bool debug;
    SrcFile* m_file;
    Module* m_module;
    std::vector<std::map<std::string, Var*>> m_vars;
    std::vector<BasicBlock*> after;
    std::vector<BasicBlock*> next;
    std::vector<SwitchInst*> _switch;
    std::vector<DIScope*> dbgScopes;
    Info* m_info;
    BasicBlock* unreach = nullptr;
    GlobalVariable* cxx_pointer_type_info = nullptr;
    GlobalVariable* cxx_class_type_info = nullptr;
    Function* cxx_pure_virtual = nullptr;
    static Generator* instance;
    static std::unique_ptr<LLVMContext> m_contxt;
    static std::vector<bool> unreachableFlag;
    static bool lastUnreachable;;
    static Struct* structRet;
    static Class* classRet;
    static bool arrRet;
    static BeContained* typeNameRet;
    static std::vector<BasicBlock*> catches;
    static std::vector<BasicBlock*> catchCont;
    static bool contained;
    static bool stump;
    static bool _final;
    static DataLayout dataLayout;
};


#endif //IGEL_COMPILER_MAINGEN_H
