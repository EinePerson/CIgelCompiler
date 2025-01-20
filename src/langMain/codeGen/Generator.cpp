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
#include <llvm/ExecutionEngine/Orc/ThreadSafeModule.h>
#include <llvm/Passes/PassBuilder.h>

#include "../langMain.hpp"
#include "../../SelfInfo.h"
#include "../../util/Mangler.h"


Generator* Generator::instance = nullptr;
std::vector<bool> Generator::unreachableFlag {};
bool Generator::lastUnreachable = false;
Struct* Generator::structRet = nullptr;
PolymorphicType* Generator::classRet = nullptr;
bool Generator::arrRet = false;
GeneratedType* const Generator::typeNameRet = new GeneratedType;
std::vector<std::unordered_map<std::string,BeContained*>> Generator::templateStack {};
std::vector<BasicBlock*> Generator::catches {};
std::vector<BasicBlock*> Generator::catchCont {};
bool Generator::contained = false;
bool Generator::stump = false;
bool Generator::_final = false;

Generator::Generator(SrcFile* file,InternalInfo* info) : m_target_triple(sys::getDefaultTargetTriple()), m_file(file),m_layout(nullptr),m_machine(nullptr),m_info(info),debug(info->flags & DEBUG_FLAG != 0),
    no_ptr_check(info->flags & NO_POINTER_CHECK != 0),no_arr_check(info->flags & NO_ARRAY_CHECK),
    m_contxt(std::make_unique<LLVMContext>()),dataLayout((new Module("",*m_contxt))->getDataLayout()) {
    m_module = std::make_unique<Module>(file->fullName, *m_contxt);
    m_builder = std::make_unique<IRBuilder<>>(*m_contxt);
    setupFlag = true;
    instance = this;
}

Generator::Generator(InternalInfo* info): m_file(nullptr), m_target_triple(sys::getDefaultTargetTriple()),m_layout(nullptr),m_machine(nullptr),m_info(info)/*,debug(false),
                                  no_ptr_check(false),no_arr_check(false)*/,m_contxt(std::make_unique<LLVMContext>()),dataLayout(getDataLayout()){
    if(info){
        debug = info->flags & DEBUG_FLAG;
        no_ptr_check = info->flags & NO_POINTER_CHECK;
        no_arr_check = info->flags & NO_ARRAY_CHECK;
    }else{
        debug = false;
        no_ptr_check = false;
        no_arr_check = false;
    }
    LLVMContext cnt;
    auto mod = new Module("dataLayout",cnt);
    dataLayout = mod->getDataLayout();
    delete mod;
    m_builder = std::make_unique<IRBuilder<>>(*m_contxt);
    instance = this;
}

void Generator::setup(SrcFile* file) {
    if (m_module)modules[file] = std::move(m_module);
    m_module = std::move(modules[file]);
    if(!m_module) {
        create(file);
        return;
    }
    if(debug) {
        dbg.unit = dbgModules[file].first;
        dbg.file = dbgModules[file].second;
        dbg.builder = new DIBuilder(*m_module,true,dbg.unit);
    }
    m_file = file;
    setupFlag = true;
    instance = this;
    m_vars.push_back({});
    instance = this;
}

void Generator::create(SrcFile *file) {
    m_module = std::make_unique<Module>(file->fullName, *m_contxt);
    m_file = file;
    if(debug) {
        dbg.builder = new DIBuilder(*m_module);
        dbg.unit = dbg.builder->createCompileUnit(dwarf::DW_LANG_C,dbg.builder->createFile(m_file->name,m_file->dir),
            ((std::string) "Igel Compiler: v") + COMPILER_VERSION,m_info->hasFlag("OPTIMIZE_FLAG"),"",0);
        dbg.file = dbg.builder->createFile(dbg.unit->getFilename(),dbg.unit->getDirectory());
        m_module->addModuleFlag(Module::Warning, "Debug Info Version",
                           DEBUG_METADATA_VERSION);
        dbgModules[file] = std::make_pair(dbg.unit,dbg.file);
    }
    modules[file] = std::move(m_module);
    setupFlag = true;
    instance = this;
    m_vars.push_back({});
}

void Generator::generateSigs() {
    if(!setupFlag) {
        std::cerr << "Usage of Uninitalized Generator\n";
        exit(EXIT_FAILURE);
    }

    m_module = std::move(modules[m_file]);

    for (auto type : m_file->types) {
        type->generateSig(m_builder.get());
    }

    for (auto type : m_file->types) {
        type->generatePart(m_builder.get());
    }

    modules[m_file] = std::move(m_module);
}

Generator::~Generator() {
    for (auto& module : modules) {
        module.second.release();
    }
}

void Generator::generate() {
    if(!setupFlag) {
        std::cerr << "Usage of Uninitialized Generator\n";
        exit(EXIT_FAILURE);
    }

    std::map<FuncSig*,Function*> funcs;

    for (auto type : m_file->types) {
        type->generate(m_builder.get());
    }

    for (const auto& [fst,snd] : m_file->funcs) {
        auto [func, sig] = snd->genFuncSig(m_builder.get());
       funcs[sig] = func;
    }

    for (const auto& [fst, snd] : m_file->funcs) {
        snd->genFunc(m_builder.get());
    }

    modules[m_file] = std::move(m_module);
}

void Generator::emitCpp(std::stringstream &ss,std::string root) {
    if(!setupFlag) {
        std::cerr << "Usage of Uninitialized Generator\n";
        exit(EXIT_FAILURE);
    }

    for (auto include : m_file->includes)ss << "#include \"" << include->fullName << "\"\n";
    for (auto include : m_file->_using)ss << "#include \"" << root << "/" << include->fullName << "\"\n";

    for (auto type : m_file->types) {
        type->writeCpp(ss);
    }

    for (const auto& [fst,snd] : m_file->funcs) {
        snd->writeCpp(ss);
    }

    modules[m_file] = std::move(m_module);
}

void Generator::emitC(std::stringstream &ss,std::string root) {
    if(!setupFlag) {
        std::cerr << "Usage of Uninitialized Generator\n";
        exit(EXIT_FAILURE);
    }

    for (auto include : m_file->includes)ss << "#include <" << include->fullName << ">\n";
    for (auto include : m_file->_using)ss << "#include <" << root << "/" << include->fullName << ">\n";

    for (auto type : m_file->types) {
        type->writeC(ss);
    }

    for (const auto& [fst,snd] : m_file->funcs) {
        snd->writeC(ss);
    }

    modules[m_file] = std::move(m_module);
}

Type* Generator::getType(TokenType type) {
    if(type <= TokenType::_ulong) {
        int i = static_cast<int>(type) % 4;
        int b = (i + 1) * 8 + 8 * std::max(i - 1,0) + 16 * std::max(i - 2,0);
        Type* t = IntegerType::get(*instance->m_contxt,b);
        return t;
    }
    if(type == TokenType::_float)return llvm::IntegerType::getFloatTy(instance->m_module->getContext());
    if(type == TokenType::_double)return llvm::IntegerType::getDoubleTy(instance->m_module->getContext());
    if(type == TokenType::_bool)return llvm::IntegerType::getInt1Ty(instance->m_module->getContext());
    if(type == TokenType::_void)return Type::getVoidTy(*instance->m_contxt);
    return nullptr;
}

void Generator::reset(SrcFile* file) {
    m_builder.release();
    m_vars.clear();
    if(debug) {
        delete dbg.builder;
    }

    m_builder = std::make_unique<IRBuilder<>>(*m_contxt);
}

void Generator::write() {
    m_vars.pop_back();
    m_module = std::move(modules[m_file]);
    if(staticInit) {
        m_builder->SetInsertPoint(staticInit);
        m_builder->CreateRetVoid();

    }
    if(debug)dbg.builder->finalize();
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

    if(m_info->hasFlag("OPTIMIZE_FLAG")) {
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

    reset();

    modules[m_file] = std::move(m_module);
    
    setupFlag = false;

}

llvm::Value* Generator::genStructVar(std::string typeName) {
    if(const auto structT = Generator::instance->m_file->findStruct(std::move(typeName),m_builder.get())) {
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

void Generator::createVar(const std::string&name,Type* type,Value* val,bool _signed,bool _final) {
    if(m_currentVar == 0) {
        if(getOptVar(name)) {
            std::cerr << "Identifier already used: " << name << std::endl;
        }
        AllocaInst* alloc = m_builder->CreateAlloca(type);
        m_builder->CreateStore(val,alloc);
        m_vars.back()[name] = new Var{alloc,_signed,_final};
    }
}

void Generator::createVar(std::string name, Var* var) {
    if(m_vars.back().contains(name)) {
        std::cerr << "Identiefier: " << name << " already in use" << std::endl;
        exit(EXIT_FAILURE);
    }
    m_vars.back()[name] = var;
}

void Generator::createVar(Argument* arg,bool _signed, const std::string&typeName,bool createDbg) {
    AllocaInst* alloc = m_builder->CreateAlloca(arg->getType()->isIntegerTy(0)?PointerType::get(*m_contxt,0):arg->getType());
    m_builder->CreateStore(arg,alloc);
    if(debug && createDbg) {
        DIType* type;
        if(auto val = m_file->findContained(typeName)) {
            if(val.has_value() && val.value()->dbgType) {
                if(val.value()->dbgpointerType)type = val.value()->dbgpointerType;
                else type = val.value()->dbgType;

            }else type = dbg.builder->createBasicType(arg->getName(),instance->m_module->getDataLayout().getTypeSizeInBits(arg->getType()),getEncodingOfType(arg->getType()));
        }else type = dbg.builder->createBasicType(arg->getName(),instance->m_module->getDataLayout().getTypeSizeInBits(arg->getType()),getEncodingOfType(arg->getType()));
        DIScope* scope = dbgScopes.empty()?dbg.file:dbgScopes.back();
        DILocalVariable* var = dbg.builder->createParameterVariable(scope,arg->getName(),arg->getArgNo(),dbg.file,0,//TODO add proper type encoding
            type);
        dbg.builder->insertDeclare(alloc,var,dbg.builder->createExpression(),DILocation::get(*m_contxt,0,0,scope),m_builder->GetInsertBlock());
    }
    if(arg->getType()->isPointerTy()) {
        if(auto structT = Generator::instance->m_file->findStruct(typeName,m_builder.get())){
            auto var = new StructVar(alloc,false);
            var->str = structT.value().second;
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
            createVar(Igel::Mangler::mangle(arg->getName().str()),var);
            return;
        }else if(auto clazzT = Generator::findClass(typeName,m_builder.get())){
            auto var = new ClassVar(alloc,false);
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
    createVar(Igel::Mangler::mangle(arg->getName().str()),new Var{alloc,_signed,false});
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
        cxx_pointer_type_info = new GlobalVariable(*Generator::instance->m_module,m_builder->getPtrTy(),false,GlobalValue::ExternalLinkage,
            nullptr,"_ZTVN10__cxxabiv119__pointer_type_infoE",
          nullptr,GlobalValue::NotThreadLocal,0,true);
    }
    if(!cxx_class_type_info) {
        cxx_class_type_info = new GlobalVariable(*Generator::instance->m_module,m_builder->getPtrTy(),false,GlobalValue::ExternalLinkage,
            nullptr,"_ZTVN10__cxxabiv117__class_type_infoE",
          nullptr,GlobalValue::NotThreadLocal,0,true);
    }
    if(!cxx_pure_virtual) {
        m_module->getOrInsertFunction("__cxa_pure_virtual",FunctionType::get(m_builder->getVoidTy(),{},false));
        cxx_pure_virtual = m_module->getFunction("__cxa_pure_virtual");
    }
}

void Generator::reset() {
    for (auto type : m_file->nameTypeMap) type.second->unregister();
    cxx_pointer_type_info = nullptr;
    cxx_class_type_info = nullptr;
    cxx_pure_virtual = nullptr;
}

Debug Generator::getDebug() {
    if(!debug)Igel::internalErr("Cannot get debug info while not in debug mode");
    return std::move(dbg);
}

int unsigned Generator::getEncodingOfType(Type *type) {
    if(type->isPointerTy()) {
        return dwarf::DW_ATE_address;
    }else if(type->isIntegerTy()) {
        return dwarf::DW_ATE_unsigned;
    }else if(type->isFloatTy() || type->isDoubleTy()) {
        return dwarf::DW_ATE_float;
    }
    return -1;
}

std::optional<std::pair<llvm::StructType*,Class*>> Generator::findClass(std::string name,llvm::IRBuilder<>* builder) {
    for (const auto &item: templateStack){
        if(item.contains(name) && dynamic_cast<Class*>(item.at(name)))return std::make_pair(dynamic_cast<Class*>(item.at(name))->strType,dynamic_cast<Class*>(item.at(name)));
    }
    return instance->m_file->findClass(name,builder);
}

std::optional<Interface*> Generator::findInterface(std::string name) {
    if(name == TEMPLATE_EMPTY_NAME)return Parser::templateEmpty;
    if (name == "Pointer")return Parser::ptr_type;
    for (const auto &item: templateStack){
        if(item.contains(name) && dynamic_cast<Interface*>(item.at(name)))return dynamic_cast<Interface*>(item.at(name));
    }
    return instance->m_file->findInterface(name);
}

std::optional<PolymorphicType *> Generator::findType(std::string name, llvm::IRBuilder<> *builder) {
    if(auto clz = findClass(name,builder))return clz->second;
    return findInterface(name);
}
