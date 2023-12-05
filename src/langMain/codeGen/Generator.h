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
    AllocaInst* alloc;
};

struct ArrayVar : Var {
    std::vector<Type*> types;
    std::vector<AllocaInst*> sizes;
};

class Generator {

public:
    explicit Generator(SrcFile* file);
    Generator();
    void setup(SrcFile* file);

    ~Generator();

    void generate();

    [[nodiscard]] Type* getType(TokenType type) const;

    std::pair<Function*,FuncSig*> genFuncSig(const IgFunction* func);

    void genFunc(IgFunction* func);

    void reset(SrcFile* file);

    void write();

    Value* exitG(Value* exitCode);

    Var* getVar(std::string name, bool _this = false);

    std::optional<Var*> getOptVar(std::string name, bool _this = false);
    void createVar(const std::string&name,Type* type,Value* val);
    void createVar(Argument* arg);
private:
    void setup();


    std::unique_ptr<IRBuilder<>> m_builder;
    bool setupFlag = false;
    std::string m_target_triple;
    DataLayout* m_layout;
    TargetMachine* m_machine;

public:
    SrcFile* m_file;
    std::unique_ptr<Module> m_module;
    std::vector<std::map<std::string, Var*>> m_vars;
    static Generator* instance;
    static std::unique_ptr<LLVMContext> m_contxt;
};


#endif //IGEL_COMPILER_MAINGEN_H
