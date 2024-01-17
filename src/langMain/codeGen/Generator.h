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

using namespace llvm;

struct SrcFile;
struct IgFunction;


struct Var {
    explicit Var(Value* alloc,bool _signed) : alloc(alloc),_signed(_signed){}
    virtual ~Var() = default;
    Value* alloc;
    char _signed = -1;
};

struct ArrayVar : Var {
    explicit ArrayVar(Value* alloc,bool _signed) : Var(alloc,_signed) {}
    std::vector<Type*> types;
    std::vector<AllocaInst*> sizes;
};

struct StructVar final : Var {
    explicit StructVar(Value* alloc) : Var(alloc,false) {}
    std::unordered_map<std::string,uint> vars;
    std::vector<Type*> types;
    std::vector<bool> signage;
    PointerType* type = nullptr;
    StructType* strType = nullptr;
};

class Generator {

public:
    explicit Generator(SrcFile* file,Info* info);
    Generator();
    ~Generator();
    void setup(SrcFile* file);

    void generate();

    [[nodiscard]] static Type* getType(TokenType type);

    std::pair<Function*,FuncSig*> genFuncSig(const IgFunction* func);

    void genFunc(IgFunction* func);

    void reset(SrcFile* file);

    void write();

    llvm::Value* genStructVar(std::string typeName);

    Value* exitG(Value* exitCode) const;
    [[nodiscard]] Value* _new() const;

    Var* getVar(const std::string&name, bool _this = false);
    std::optional<Var*> getOptVar(std::string name, bool _this = false);

    void createVar(const std::string&name,Type* type,Value* val,bool _signed);
    void createVar(Argument* arg,bool _signed);
private:

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
};


#endif //IGEL_COMPILER_MAINGEN_H
