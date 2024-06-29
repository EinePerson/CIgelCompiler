//
// Created by igel on 16.11.23.
//

#include "Generator.h"

#include <llvm/IR/LLVMContext.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/IR/Verifier.h>
#include <llvm/Support/raw_os_ostream.h>
#include <llvm/Support/InitLLVM.h>
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Type.h"
#include "llvm/MC/TargetRegistry.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Support/TargetSelect.h"
#include "llvm/Target/TargetMachine.h"
#include "llvm/Target/TargetOptions.h"
#include "llvm/TargetParser/Host.h"
#include <algorithm>
#include <cassert>
#include <cstdlib>
#include <map>
#include <memory>
#include <string>
#include <system_error>
#include <utility>
#include <vector>
#include <llvm/Bitcode/BitcodeWriter.h>
#include <llvm/Passes/PassBuilder.h>

#include "../langMain.hpp"
#include "../../util/Mangler.h"


Generator* Generator::instance = nullptr;
std::vector<bool> Generator::unreachableFlag {};
std::unique_ptr<LLVMContext> Generator::m_contxt = std::make_unique<LLVMContext>();
bool Generator::lastUnreachable = false;
Struct* Generator::structRet = nullptr;
Class* Generator::classRet = nullptr;
bool Generator::arrRet = false;
BeContained* Generator::typeNameRet = nullptr;
std::vector<BasicBlock*> Generator::catches {};
std::vector<BasicBlock*> Generator::catchCont {};
bool Generator::contained = false;
bool Generator::stump = false;

Generator::Generator(SrcFile* file,Info* info) : m_target_triple(sys::getDefaultTargetTriple()), m_file(file),m_layout(nullptr),m_machine(nullptr),m_info(info) {
    m_module = std::make_unique<Module>(file->fullName, *m_contxt);
    m_builder = std::make_unique<IRBuilder<>>(*m_contxt);
    setupFlag = true;
    instance = this;
}

Generator::Generator(): m_file(nullptr), m_target_triple(sys::getDefaultTargetTriple()),m_layout(nullptr),m_machine(nullptr),m_info(nullptr) {
    m_builder = std::make_unique<IRBuilder<>>(*m_contxt);
}

void Generator::setup(SrcFile* file) {
    m_module = std::make_unique<Module>(file->fullName, *m_contxt);
    m_file = file;
    setupFlag = true;
    instance = this;
    m_vars.push_back({});
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
        type->generatePart(m_builder.get());
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
}

Type* Generator::getType(TokenType type) {
    if(type <= TokenType::_ulong) {
        int i = static_cast<int>(type) % 4;
        int b = (i + 1) * 8 + 8 * std::max(i - 1,0) + 16 * std::max(i - 2,0);
        Type* t = IntegerType::get(*m_contxt,b);
        return t;
    }
    if(type == TokenType::_float)return llvm::IntegerType::getFloatTy(instance->m_module->getContext());
    if(type == TokenType::_double)return llvm::IntegerType::getDoubleTy(instance->m_module->getContext());
    if(type == TokenType::_bool)return llvm::IntegerType::getInt1Ty(instance->m_module->getContext());
    if(type == TokenType::_void)return Type::getVoidTy(*m_contxt);
    return nullptr;
}



std::pair<Function*,FuncSig*> Generator::genFuncSig(IgFunction* func) {
    std::vector<Type*> types;
    for(uint i = 0;i < func->paramType.size();i++) {
        types.push_back(func->paramType[i]);
    }
    const ArrayRef<Type*> ref(types);
    FunctionType* type = FunctionType::get(func->_return,ref,false);
    llvm::Function* llvmFunc = Function::Create(type,Function::ExternalLinkage,func->mangle(),m_module.get());
    for(size_t i = 0;i < llvmFunc->arg_size();i++)
        llvmFunc->getArg(i)->setName(func->paramName[llvmFunc->getArg(i)->getArgNo()]);
    return std::make_pair(llvmFunc,new FuncSig(func->mangle(),types,func->_return));
}

void Generator::genFunc(IgFunction* func,bool member) {
    Function* llvmFunc = m_module->getFunction(func->mangle());
    BasicBlock* entry = BasicBlock::Create(*m_contxt,"entry",llvmFunc);
    m_builder->SetInsertPoint(entry);

    m_vars.emplace_back();
    if(func->supper.has_value()) {
        llvmFunc->getArg(0)->setName("this");
        createVar(llvmFunc->getArg(0),false,func->supper.value()->mangle());
    }
    for(size_t i = func->supper.has_value();i < llvmFunc->arg_size();i++){
        llvmFunc->getArg(i)->setName(func->paramName[i]);
        createVar(llvmFunc->getArg(i),func->signage[i],llvmFunc->getArg(i)->getType()->isPointerTy()?(func->paramTypeName[i]?func->paramTypeName[i]->mangle():"_Z4this"):"");
    }

    func->scope->generate(m_builder.get());
    m_vars.pop_back();
    if(func->_return == Type::getVoidTy(*m_contxt))m_builder->CreateRetVoid();
    if(unreach) {
        m_builder->SetInsertPoint(unreach);
        m_builder->CreateUnreachable();
        unreach = nullptr;
    }
    if(llvm::verifyFunction(*llvmFunc,&outs())) {
        m_module->print(outs(), nullptr);
        exit(EXIT_FAILURE);
    }
    func->llvmFunc = llvmFunc;
}

void Generator::reset(SrcFile* file) {
    m_builder.release();
    //m_contxt.release();
    m_module.release();
    m_vars.clear();


    m_module = std::make_unique<Module>(file->fullName, *m_contxt);
    m_builder = std::make_unique<IRBuilder<>>(*m_contxt);
}

void Generator::write() {
    m_vars.pop_back();
    if(staticInit) {
        m_builder->SetInsertPoint(staticInit);
        m_builder->CreateRetVoid();

        /*FunctionType* type = FunctionType::get(m_builder->getVoidTy(),{},false);
        FunctionCallee callee = m_module->getOrInsertFunction("_GLOBAL__sub_I_example.cpp",type);*/
    }
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

    if(verifyModule(*m_module,&outs())) {
        m_module->print(outs(),nullptr);
        exit(EXIT_FAILURE);
    }

    if(m_info->hasFlag("Optimize")) {
        PassBuilder PB;
        PB.registerModuleAnalyses(MAM);
        PB.registerCGSCCAnalyses(CGAM);
        PB.registerFunctionAnalyses(FAM);
        PB.registerLoopAnalyses(LAM);
        PB.crossRegisterProxies(LAM, FAM, CGAM, MAM);
        ModulePassManager MPM = PB.buildPerModuleDefaultPipeline(OptimizationLevel::O2);
        MPM.run(*m_module,MAM);
    }

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
    if(const auto structT = Generator::instance->m_file->findStruct(std::move(typeName))) {
        m_currentVar++;
        m_sVarId.push_back(0);
        m_vars.emplace_back();
        Value* var = m_builder->CreateAlloca(structT.value().first);
        std::unordered_map<std::string,uint> vars;
        for (size_t i = 0;i < structT.value().second->vars.size();i++) {
            if(!structT.value().second->vars[i]->_static) {
                Value* val = m_builder->CreateStructGEP(structT.value().first,var,m_sVarId.back());
                Value* gen = structT.value().second->vars[i]->generate(m_builder.get());
                m_builder->CreateStore(gen,val);
                m_sVarId.back()++;
                std::string str = structT.value().second->vars[i]->mangle();
                vars[str] = i;
            }else structT.value().second->vars[i]->generate(m_builder.get());
        }
        m_currentVar--;
        m_sVarId.pop_back();
        m_vars.pop_back();
        if(structT.value().second->varIdMs.empty())structT.value().second->varIdMs = vars;
        if(!structT.value().second->strType)structT.value().second->strType = structT.value().first;
        return var;
    }
    std::cerr << "Unknown type " << typeName << std::endl;
    exit(EXIT_FAILURE);
}

auto Generator::exitG(Value* exitCode) const -> Value* {
    std::vector<Type*> params = {Type::getInt32Ty(*m_contxt)};
    FunctionType *fType = FunctionType::get(Type::getVoidTy(*m_contxt), params, false);
    llvm::FunctionCallee exitFunc = m_module->getOrInsertFunction("exit",fType);
    Value* ret = m_builder->CreateCall(exitFunc, {exitCode});

    return ret;
}

Value* Generator::_new() const {
    FunctionType* type = FunctionType::get(PointerType::get(*m_contxt,0),{Type::getInt64Ty(*m_contxt)},false);
    const llvm::FunctionCallee _new = m_module->getOrInsertFunction("_Znwm",type);
    return m_builder->CreateCall(_new,ConstantInt::get(Type::getInt64Ty(*m_contxt),0));
}


Var* Generator::getVar(const std::string&name, bool _this){
    const std::optional<Var*> opt = getOptVar(name,_this);
    if(!opt.has_value()) {
        std::cerr << "Undeclared identifier: " << name << std::endl;
        exit(EXIT_FAILURE);
    }
    return opt.value();
}

std::optional<Var*> Generator::getOptVar(std::string name, bool _this) {
    if(_this) {
        if(m_vars[1].contains(name))return m_vars[1][name];
        return {};
    }
    auto map = std::find_if(m_vars.rbegin(), m_vars.rend(), [&](const std::map<std::string,Var*> mapl) -> Var* {
        if(mapl.contains(name))return mapl.at(name);
        return nullptr;
    });
    if(map != m_vars.rend() && map->contains(name)) {
        return map->at(name);
    }
    return  {};
}

void Generator::createVar(const std::string&name,Type* type,Value* val,bool _signed) {
    if(m_currentVar == 0) {
        if(getOptVar(name)) {
            std::cerr << "Identifier already used: " << name << std::endl;
        }
        AllocaInst* alloc = m_builder->CreateAlloca(type);
        m_builder->CreateStore(val,alloc);
        m_vars.back()[name] = new Var{alloc,_signed};
    }
}

void Generator::createVar(std::string name, Var* var) {
    if(m_vars.back().contains(name)) {
        std::cerr << "Identiefier: " << name << " already in use" << std::endl;
        exit(EXIT_FAILURE);
    }
    m_vars.back()[name] = var;
}

void Generator::createVar(Argument* arg,bool _signed, const std::string&typeName) {
    AllocaInst* alloc = m_builder->CreateAlloca(arg->getType()->isIntegerTy(0)?PointerType::get(*m_contxt,0):arg->getType());
    m_builder->CreateStore(arg,alloc);
    if(arg->getType()->isPointerTy()) {
        if(auto structT = Generator::instance->m_file->findStruct(typeName)){
            auto var = new StructVar(alloc);
            var->str = structT.value().second;
            //var->type = arg->getType()->isPointerTy()?static_cast<PointerType*>(arg->getType()):PointerType::get(*m_contxt,0);
            var->strType = structT.value().first;
            var->types = structT.value().second->types;
            for(size_t i = 0;i < structT.value().second->vars.size();i++) {
                NodeStmtLet* let = structT.value().second->vars[i];
                std::string str = structT.value().second->vars[i]->mangle();
                if(auto pirim = dynamic_cast<NodeStmtPirimitiv*>(let))var->signage.push_back(pirim->sid <= (char) TokenType::_long);
                else var->signage.push_back(false);
                var->vars[str] = i;
            }


            if(structT.value().second->varIdMs.empty())structT.value().second->varIdMs = var->vars;
            if(!structT.value().second->strType)structT.value().second->strType = var->strType;
            createVar(Igel::Mangler::mangle(arg->getName().str()),var);
            return;
        }else if(auto clazzT = Generator::instance->m_file->findClass(typeName)){
            auto var = new ClassVar(alloc);
            var->clazz = clazzT.value().second;
            var->type = arg->getType()->isPointerTy()?static_cast<PointerType*>(arg->getType()):PointerType::get(*m_contxt,0);
            var->strType = clazzT.value().first;
            var->types = clazzT.value().second->types;
            for(size_t i = 0;i < clazzT.value().second->vars.size();i++) {
                NodeStmtLet* let = clazzT.value().second->vars[i];
                std::string str = clazzT.value().second->vars[i]->name;
                if(auto pirim = dynamic_cast<NodeStmtPirimitiv*>(let))var->signage.push_back(pirim->sid <= (char) TokenType::_long);
                else var->signage.push_back(false);
                var->vars[str] = i + 1;
            }


            if(clazzT.value().second->varIdMs.empty())clazzT.value().second->varIdMs = var->vars;
            if(!clazzT.value().second->strType)clazzT.value().second->strType = var->strType;
            createVar(Igel::Mangler::mangle(arg->getName().str()),var);
            return;
        }
    }
    createVar(Igel::Mangler::mangle(arg->getName().str()),new Var{alloc,_signed});
}

void Generator::createStaticVar(std::string name, Value* val,Var* var) {
    if(!staticInit) {
        FunctionType* ty = llvm::FunctionType::get(m_builder->getVoidTy(),{},false);
        m_module->getOrInsertFunction("__cxx_global_var_init",ty);
        Function* func = m_module->getFunction("__cxx_global_var_init");
        func->setSection(".text.startup");
        staticInit = BasicBlock::Create(*m_contxt,"staticInit",func);

        StructType* str = StructType::create({m_builder->getInt32Ty(),m_builder->getPtrTy(),m_builder->getPtrTy()});
        ArrayType* arr = ArrayType::get(str,1);
        Constant* constArr = ConstantArray::get(arr,ConstantStruct::get(str,{ConstantInt::get(m_builder->getInt32Ty(),65535),func,ConstantPointerNull::get(m_builder->getPtrTy())}));
        new GlobalVariable(*m_module,arr,true,GlobalValue::AppendingLinkage,constArr,"llvm.global_ctors");
    }
    m_builder->SetInsertPoint(staticInit);
    m_builder->CreateStore(val,m_module->getGlobalVariable(name));
    if(m_vars.back().contains(name)) {
        std::cerr << "Identiefier: " << name << " already in use" << std::endl;
        exit(EXIT_FAILURE);
    }
    m_vars.back()[name] = var;
}

void Generator::initInfo() {
    if(!cxx_pointer_type_info) {
        cxx_pointer_type_info = new GlobalVariable(*Generator::instance->m_module,m_builder->getPtrTy(),false,GlobalValue::ExternalLinkage,nullptr,"_ZTVN10__cxxabiv119__pointer_type_infoE",
          nullptr,GlobalValue::NotThreadLocal,0,true);;
    }
    if(!cxx_class_type_info) {
        cxx_class_type_info = new GlobalVariable(*Generator::instance->m_module,m_builder->getPtrTy(),false,GlobalValue::ExternalLinkage,nullptr,"_ZTVN10__cxxabiv117__class_type_infoE",
          nullptr,GlobalValue::NotThreadLocal,0,true);
    }
    if(!cxx_pure_virtual) {
        m_module->getOrInsertFunction("__cxa_pure_virtual",FunctionType::get(m_builder->getVoidTy(),{},false));
        cxx_pure_virtual = m_module->getFunction("__cxa_pure_virtual");
    }
}
