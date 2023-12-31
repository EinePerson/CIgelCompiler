//
// Created by igel on 16.11.23.
//

#include "Generator.h"

#include <llvm/IR/LLVMContext.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/IR/Verifier.h>
#include <llvm/Support/raw_os_ostream.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/Support/InitLLVM.h>
#include "llvm/ADT/APFloat.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Type.h"
#include "llvm/IR/Verifier.h"
#include "llvm/MC/TargetRegistry.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Support/TargetSelect.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Target/TargetMachine.h"
#include "llvm/Target/TargetOptions.h"
#include "llvm/TargetParser/Host.h"
#include <algorithm>
#include <cassert>
#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <map>
#include <memory>
#include <string>
#include <system_error>
#include <utility>
#include <vector>
#include <llvm/Bitcode/BitcodeWriter.h>
#include <llvm/Passes/PassBuilder.h>


Generator* Generator::instance = nullptr;
std::unique_ptr<LLVMContext> Generator::m_contxt = std::make_unique<LLVMContext>();

Generator::Generator(SrcFile* file) : m_file(file), m_target_triple(sys::getDefaultTargetTriple()) {
    //m_contxt = std::make_unique<LLVMContext>();
    m_module = std::make_unique<Module>(file->fullName, *m_contxt);
    m_builder = std::make_unique<IRBuilder<>>(*m_contxt);
    setup();
    setupFlag = true;
    instance = this;
}

Generator::Generator(): m_file(nullptr), m_target_triple(sys::getDefaultTargetTriple()) {
    //m_contxt = std::make_unique<LLVMContext>();
    m_builder = std::make_unique<IRBuilder<>>(*m_contxt);
    setup();
}

void Generator::setup() {

    /*m_module->setTargetTriple(m_target_triple);
    std::string Error;
    auto Target = TargetRegistry::lookupTarget(m_target_triple, Error);

    // Print an error and exit if we couldn't find the requested target.
    // This generally occurs if we've forgotten to initialise the
    // TargetRegistry or we have a bogus target triple.
    if (!Target) {
        errs() << Error;
        exit(EXIT_FAILURE);
    }
    auto CPU = "generic";
    auto Features = "";

    TargetOptions opt;
    auto RM = std::optional<Reloc::Model>();
    m_machine = Target->createTargetMachine(m_target_triple, CPU, Features, opt, RM);
    DataLayout l = m_machine->createDataLayout();
    m_layout = &l;
    m_module->setDataLayout(*m_layout);*/
}

void Generator::setup(SrcFile* file) {
    //if(setupFlag)return;
    m_module = std::make_unique<Module>(file->fullName, *m_contxt);
    m_file = file;
    /*m_module->setTargetTriple(m_target_triple);
    m_module->setDataLayout(*m_layout);*/
    setupFlag = true;
    instance = this;
}

Generator::~Generator() = default;

void Generator::generate() {
    if(!setupFlag) {
        std::cerr << "Usage of Uninitalized Generator\n";
        exit(EXIT_FAILURE);
    }

    std::map<FuncSig*,Function*> funcs;

    for (auto type : m_file->types) {
        type->generateSig(m_builder.get());
    }

    for (auto type : m_file->types) {
        type->generate(m_builder.get());
    }

    for (const auto& [fst,snd] : m_file->funcs) {
        auto [func, sig] = genFuncSig(snd);
       funcs[sig] = func;
    }

    for (const auto& [fst, snd] : m_file->funcs) {
        genFunc(snd);
    }

    for (const auto stmt : m_file->stmts) {
        stmt->generate(m_builder.get());
    }
}

Type* Generator::getType(TokenType type) {
    if(type <= TokenType::_ulong) {
        int i = static_cast<int>(type) % 4;
        int b = (i + 1) * 8 + 8 * std::max(i - 1,0) + 16 * std::max(i - 2,0);
        Type* t = IntegerType::get(*m_contxt,b);
        return t;
    }
    if(type == TokenType::_void)return Type::getVoidTy(*m_contxt);
    return nullptr;
}



std::pair<Function*,FuncSig*> Generator::genFuncSig(const IgFunction* func) {
    std::vector<Type*> types;
    for (const auto param_type : func->paramType) {
        types.push_back(param_type);
    }
    const ArrayRef<Type*> ref(types);
    FunctionType* type = FunctionType::get(func->_return,ref,false);
    llvm::Function* llvmFunc = Function::Create(type,Function::ExternalLinkage,func->name,m_module.get());
    for(size_t i = 0;i < llvmFunc->arg_size();i++)
        llvmFunc->getArg(i)->setName(func->paramName[llvmFunc->getArg(i)->getArgNo()]);
    return std::make_pair(llvmFunc,new FuncSig(func->name,types,func->_return));
}

void Generator::genFunc(IgFunction* func) {
    Function* llvmFunc = m_module->getFunction(func->name);
    BasicBlock* entry = BasicBlock::Create(*m_contxt,"entry",llvmFunc);
    m_builder->SetInsertPoint(entry);

    m_vars.emplace_back();
    for(size_t i = 0;i < llvmFunc->arg_size();i++){
        createVar(llvmFunc->getArg(i));
    }

    func->scope->generate(m_builder.get());
    m_vars.pop_back();
    if(func->_return == Type::getVoidTy(*m_contxt))m_builder->CreateRetVoid();
    if(llvm::verifyFunction(*llvmFunc,&outs())) {
        //m_module->print(outs(),nullptr);
        exit(EXIT_FAILURE);
    }
}

void Generator::reset(SrcFile* file) {
    m_builder.release();
    //m_contxt.release();
    m_module.release();
    m_vars.clear();


    //m_contxt = std::make_unique<LLVMContext>();
    m_module = std::make_unique<Module>(file->fullName, *m_contxt);
    m_builder = std::make_unique<IRBuilder<>>(*m_contxt);
}

void Generator::write() {
    InitializeAllTargetInfos();
    InitializeAllTargets();
    InitializeAllTargetMCs();
    InitializeAllAsmParsers();
    InitializeAllAsmPrinters();

    verifyModule(*m_module);

    auto TargetTriple = sys::getDefaultTargetTriple();
    m_module->setTargetTriple(TargetTriple);

    std::string Error;
    auto Target = TargetRegistry::lookupTarget(TargetTriple, Error);

    if (!Target) {
        errs() << Error;
        exit(1);
    }

    auto CPU = "generic";
    auto Features = "";

    TargetOptions opt;
    auto RM = std::optional<Reloc::Model>();
    auto m_machine =
        Target->createTargetMachine(TargetTriple, CPU, Features, opt, RM);

    m_module->setDataLayout(m_machine->createDataLayout());

    LoopAnalysisManager LAM;
    FunctionAnalysisManager FAM;
    CGSCCAnalysisManager CGAM;
    ModuleAnalysisManager MAM;

    if(verifyModule(*m_module,&outs()))exit(EXIT_FAILURE);

    PassBuilder PB;
    PB.registerModuleAnalyses(MAM);
    PB.registerCGSCCAnalyses(CGAM);
    PB.registerFunctionAnalyses(FAM);
    PB.registerLoopAnalyses(LAM);
    PB.crossRegisterProxies(LAM, FAM, CGAM, MAM);
    ModulePassManager MPM = PB.buildPerModuleDefaultPipeline(OptimizationLevel::O2);
    MPM.run(*m_module,MAM);

    //m_module->print(outs(),nullptr);

    std::error_code EC;
    raw_fd_ostream dest("./build/cmp/" + m_file->fullName + ".bc", EC, sys::fs::OF_None);
    if (EC) {
        errs() << "Could not open file: " << EC.message();
        exit(EXIT_FAILURE);
    }
    WriteBitcodeToFile(*m_module, dest);
    dest.flush();

    setupFlag = false;

}

llvm::Value* Generator::genStructVar(std::string typeName) {
    /*if(auto structT = Generator::instance->m_file->findStruct(std::move(name))) {
        //auto type = PointerType::get(structT.value().first,0);
        auto type = PointerType::get(*m_contxt,0);
        //auto type = structT.value().first;
        AllocaInst* alloc = m_builder->CreateAlloca(type);
        Value* gen = term->generate(m_builder.get());
        m_builder->CreateStore(/*this->_new()*//*genllvm::ConstantPointerNull::get(PointerType::get(*m_contxt,0)),alloc);

        auto var = new StructVar(alloc);
        var->type = type;
        var->strType = structT.value().first;
        m_currentVar.push_back(var);
        m_sVarId.push_back(0);

        var->types.reserve(structT.value().second->vars.size());
        /*for(size_t i = 0;i < structT.value().first->getNumElements();i++) {
            structT.value().first->getElementType(i)->print(outs(),true);
        }

        for (auto node_stmt_let : structT.value().second->vars) {
            Value* ptr = m_builder->CreateLoad(PointerType::get(*m_contxt,0),var->alloc);
            //m_module->print(outs(),nullptr);
            //std::cout << "\n\n";
            //structT.value().first->getElementType(m_sVarId.back())->print(outs());
            //std::cout << structT.value().first->getNumElements() << "A\n";
            Value* val = m_builder->CreateStructGEP(structT.value().first,ptr,m_sVarId.back());
            Value* gen = node_stmt_let->generate(m_builder.get());
            m_builder->CreateStore(gen,val);
            //m_varLast.back()->alloc = val;
            //var->varPtrs[node_stmt_let->ident.value.value()] = m_varLast.back();
            //m_varLast.pop_back();
            var->types.push_back(gen->getType());
            m_sVarId.back()++;
        }

        m_currentVar.pop_back();
        m_sVarId.pop_back();
        m_vars.back()[id.value.value()] = var;
    }*/

    if(auto structT = Generator::instance->m_file->findStruct(std::move(typeName))) {
        //m_currentVar.push_back(var);
        m_currentVar++;
        m_sVarId.push_back(0);
        FunctionType* type = FunctionType::get(PointerType::get(*m_contxt,0),{Type::getInt64Ty(*m_contxt)},false);
        FunctionCallee _new = m_module->getOrInsertFunction("_Znwm",type);
        Value* var = m_builder->CreateCall(_new,ConstantInt::get(Type::getInt64Ty(*m_contxt),0));
        for (auto node_stmt_let : structT.value().second->vars) {
            Value* val = m_builder->CreateStructGEP(structT.value().first,var,m_sVarId.back());
            Value* gen = node_stmt_let->generate(m_builder.get());
            m_builder->CreateStore(gen,val);
            m_sVarId.back()++;
        }
        m_currentVar--;
        m_sVarId.pop_back();
        return var;
    }
    std::cerr << "Unknown type " << typeName << std::endl;
    exit(EXIT_FAILURE);
}

auto Generator::exitG(Value* exitCode) const -> Value* {
    std::vector<Type*> params = {Type::getInt32Ty(*m_contxt)};
    FunctionType *fType = FunctionType::get(Type::getVoidTy(*m_contxt), params, false);
    FunctionCallee exitFunc = m_module->getOrInsertFunction("exit",fType);
    Value* ret = m_builder->CreateCall(exitFunc, {exitCode});
    //m_builder->CreateUnreachable();
    return ret;
}

Value* Generator::_new() const {
    FunctionType* type = FunctionType::get(PointerType::get(*m_contxt,0),{Type::getInt64Ty(*m_contxt)},false);
    FunctionCallee _new = m_module->getOrInsertFunction("_Znwm",type);
    return m_builder->CreateCall(_new,ConstantInt::get(Type::getInt64Ty(*m_contxt),0));
}


Var* Generator::getVar(std::string name, bool _this) {
    std::optional<Var*> opt = getOptVar(name,_this);
    if(!opt.has_value()) {
        std::cerr << "Undeclared identifier: " << name << std::endl;
        exit(EXIT_FAILURE);
    }
    return opt.value();
}

std::optional<Var*> Generator::getOptVar(std::string name, bool _this) {
    if(_this) {
        if(m_vars[0].contains(name))return m_vars[0][name];
        return {};
    }
    auto map = std::find_if(m_vars.rbegin(), m_vars.rbegin(), [&](const std::map<std::string,Var*> mapl) {
        if(mapl.contains(name))return mapl.at(name);
    });
    if(map != m_vars.rend() && map->contains(name)) {
        return map->at(name);
    }
    return  {};
}

/*Var* Generator::getVar(const std::vector<std::string>&names, const bool _this) {
    std::optional<Var*> opt = getOptVar(names,_this);
    if(!opt.has_value()) {
        std::cerr << "Undeclared identifier: ";
        for (auto name : names) std::cerr << name;
        std::cerr << std::endl;
        exit(EXIT_FAILURE);
    }
return opt.value();
}*/

/*std::optional<Var*> Generator::getOptVar(const std::vector<std::string>&names, bool _this) {
    if(names.empty())return {};
    StructVar* str = nullptr;
    if(auto var = getOptVar(names[0])) {
        if(auto _struct = dynamic_cast<StructVar*>(var.value()))str = _struct;
        else return {};
    }else return {};
    for(size_t i = 1;i < names.size() - 1;i++) {
        if(str->varPtrs.contains(names[i]))str = dynamic_cast<StructVar*>(str->varPtrs[names[i]]);
        else return {};
    }
    if(str->varPtrs.contains(names.back()))return str->varPtrs[names.back()];
    return {};
}*/

void Generator::createVar(const std::string&name,Type* type,Value* val) {
    if(m_currentVar == 0) {
        if(getOptVar(name)) {
            std::cerr << "Identifier already used: " << name << std::endl;
        }
        AllocaInst* alloc = m_builder->CreateAlloca(type);
        m_builder->CreateStore(val,alloc);
        m_vars.back()[name] = new Var{alloc};
    }/*else {
        m_currentVar.back()->vars[name] = m_sVarId.back();
        //m_varLast.push_back(new Var(nullptr));
    }*/
}

void Generator::createVar(Argument* arg) {
    //if(m_currentVar.empty()) {
        AllocaInst* alloc = m_builder->CreateAlloca(arg->getType());
        m_builder->CreateStore(arg,alloc);
        m_vars.back()[static_cast<std::string>(arg->getName())] = new Var{alloc};
    /*}else {
        m_currentVar.back()->vars[static_cast<std::string>(arg->getName())] = m_sVarId.back();
        //currentVar.back()->vars[static_cast<std::string>(arg->getName())] = new Var{alloc}
    }*/
}




