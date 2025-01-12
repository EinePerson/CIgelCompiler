//
// Created by igel on 19.11.23.
//

#include <llvm/IR/Verifier.h>
#include <llvm/Support/CommandLine.h>
#include <llvm/Support/JSON.h>

#include "Generator.h"
#include "../../util/Mangler.h"

#include <string>
#include <iostream>

#include "../../exceptionns/GeneratedException.h"
#include "../../types.h"


//Name for the specifier of an array length
#define ARR_LENGTH "length"

llvm::Value* NodeTermId::generate(llvm::IRBuilder<>* builder) {
     if(contained) {
          return contained.value()->generate(builder);
     }
     if(cont->contType.has_value() && dynamic_cast<Enum*>(cont->contType.value())) {
          auto _enum = dynamic_cast<Enum*>(cont->contType.value());
          return ConstantInt::get(builder->getInt32Ty(),_enum->valueIdMap[cont->name]);
     }
     Var* var = Generator::instance->getVar(Igel::Mangler::mangleName(cont));
     if(auto str = dynamic_cast<StructVar*>(var)) {
          Generator::setTypeNameRet(str->str);
          Generator::structRet = str->str;
          Generator::classRet = nullptr;
     }
     _signed = var->_signed;
     if(auto clazz = dynamic_cast<ClassVar*>(var)) {
          Generator::setTypeNameRet(clazz->clazz);
     }
     return Igel::setDbg(builder,builder->CreateLoad(var->getType(),var->alloc),pos);
}

llvm::Value* NodeTermId::generatePointer(llvm::IRBuilder<>* builder) {
     if(contained) {
          return contained.value()->generatePointer(builder);
     }
     Var* var = Generator::instance->getVar(Igel::Mangler::mangleName(cont));
     Generator::_final = var->_final;
     if(auto str = dynamic_cast<StructVar*>(var)){
          Generator::setTypeNameRet(str->str);
          Generator::structRet = str->str;
          Generator::classRet = nullptr;
     }
     Value* ptr = var->alloc;
     if(auto clazz = dynamic_cast<ClassVar*>(var)) {
          Generator::setTypeNameRet(clazz->clazz);
     }
     return ptr;
}

llvm::Value * NodeTermAcces::generate(llvm::IRBuilder<> *builder) {
     Generator::arrRet = false;
     if(call) {
          Value* val = call->generate(builder);
          if(auto func = Generator::instance->m_file->findIgFunc(call->name->mangle(),call->params)) {
               auto clz = dynamic_cast<Class*>(func.value()->retTypeName->type);
               auto str = dynamic_cast<Struct*>(func.value()->retTypeName->type);
               if(!clz || !str)return nullptr;

               if(clz)Generator::setTypeNameRet(clz);
               else Generator::setTypeNameRet(str);

               if(acc.value.value() == ARR_LENGTH && Generator::arrRet) {
                    return  builder->CreateStructGEP(clz?clz->strType:str->getStrType(builder),val,0);
               }
               if(clz) {
                    if(!Igel::SecurityManager::canAccess(clz,superType,clz->varAccesses[acc.value.value()])) {
                         Igel::err("Cannot access: " + acc.value.value(),acc);
                    }
               }
               uint fid = clz?clz->varIdMs[acc.value.value()]:str->varIdMs[acc.value.value()];
               Generator::_final = clz?clz->finals[acc.value.value()]:str->finals[acc.value.value()];
               return Igel::setDbg(builder,builder->CreateLoad(clz?clz->types[fid]:str->types[fid],this->generatePointer(builder)),(Position) acc);

          }else return nullptr;
     }

     Value* val;
     bool isClz;

     Struct* str = nullptr;
     StructVar* var = nullptr;

     Class* clz = nullptr;
     ClassVar* clzVar = nullptr;
     if(contained) {
          Generator::contained = true;
          val = contained.value()->generatePointer(builder);
          if(Generator::structRet) {
               str = Generator::structRet;
               Generator::_final = str->finals[acc.value.value()];
               Generator::structRet = nullptr;
               isClz = false;
          }else {
               Generator::contained = true;
               val = contained.value()->generate(builder);
               if(!dynamic_cast<Class*>(Generator::classRet))Igel::err("This opperation can only be performed on classes",pos);
               clz = dynamic_cast<Class*>(Generator::classRet);
               Generator::_final = clz->finals[acc.value.value()];
               Generator::classRet = nullptr;
               isClz = true;
          }
          Generator::contained = false;
     }else {
          if(var = dynamic_cast<StructVar*>(Generator::instance->getVar(id->mangle()))) {
               Generator::_final = var->str->finals[acc.value.value()];
               val = var->alloc;
               isClz = false;
          }else if(clzVar = dynamic_cast<ClassVar*>(Generator::instance->getVar(id->mangle()))){
               Generator::_final = dynamic_cast<Class*>(clzVar->clazz)?dynamic_cast<Class*>(clzVar->clazz)->finals[acc.value.value()]:false;
               val = Igel::setDbg(builder,builder->CreateLoad(clzVar->type,clzVar->alloc),pos);
               isClz = true;
          }else {
               auto arrVar = dynamic_cast<ArrayVar*>(Generator::instance->getVar(id->mangle()));
               if(!arrVar || acc.value.value() != ARR_LENGTH) {
                    Igel::err("Expected array length",acc);
               }

               val = arrVar->alloc;
               //if(arrVar->typeName.has_value() && Generator::instance->m_file->findClass(arrVar->typeName.value()->mangle(),builder))
               val = Igel::setDbg(builder,builder->CreateLoad(builder->getPtrTy(),val),pos);
               Igel::check_Pointer(builder,val);
               val = builder->CreateInBoundsGEP(val->getType(),val,builder->getInt64(0));
               return Igel::setDbg(builder,builder->CreateLoad(builder->getInt64Ty(),val),(Position) acc);
          }
     }

     if(!(str || var || clz || clzVar)) {
          Igel::err("Neither contained value nor variable",acc);
     }
     Igel::check_Pointer(builder,val);

     if(!isClz) {
          uint fid = var ? var->vars[acc.value.value()]:str->varIdMs[acc.value.value()];
          if(acc.value.value() == ARR_LENGTH && Generator::arrRet)fid = 0;
          Value* ptr = builder->CreateStructGEP(var?var->strType:str->getStrType(builder),val,fid);
          Generator::setTypeNameRet(str?str:var->str);
          if(var) {
               Generator::structRet = var->str;
               Generator::classRet = nullptr;
               return Igel::setDbg(builder,builder->CreateLoad(var->types[fid],ptr),(Position) acc);
          }

          if(str->typeName[fid] != nullptr) {
               if(const auto strT = Generator::instance->m_file->findStruct(str->typeName[fid]->mangle(),builder)) {
                    Generator::structRet = strT.value().second;
                    Generator::classRet = nullptr;
               }
               else Generator::structRet = nullptr;
          }else Generator::structRet = nullptr;
          return ptr;
     }else {

          if(!dynamic_cast<Class*>(clzVar?clzVar->clazz:clz)) {
               Igel::err("Can only access fields of classes",acc);
          }
          std::vector<PolymorphicType*> templateVals = clzVar?clzVar->templateVals:Generator::typeNameRet->templateTypes;
          if(!clz)clz = dynamic_cast<Class*>(clzVar->clazz);
          Value* ptr = nullptr;
          auto ret = clz->getVarialbe(acc.value.value());
          if(!ret.first) {
               Igel::err("Unkown variable name: " + acc.value.value(),acc);
          }
          uint fid = ret.second;
          if(acc.value.value() == ARR_LENGTH && Generator::arrRet)fid = 0;

          if(!Igel::SecurityManager::canAccess(ret.first,superType,ret.first->varAccesses[acc.value.value()])) {
               Igel::err("Cannot access: " + acc.value.value(),acc);
          }

          ptr = builder->CreateStructGEP(ret.first->strType,val,fid);
          Generator::setTypeNameRet(clz,templateVals);
          if(Generator::contained) {
               if (clz->templateIdMs.contains(acc.value.value()) && !templateVals.empty()) {
                    Generator::classRet = templateVals[clz->templateIdMs[acc.value.value()]];
               }else if(const auto strT = Generator::findType(ret.first->vars[fid - ret.first->vTablePtrs]->typeName->mangle(),builder)) {
                    Generator::classRet = strT.value();
                    Generator::structRet = nullptr;
               }
               Generator::contained = false;
          }else {
               if (clz->templateIdMs.contains(acc.value.value()) && !templateVals.empty()) {
                        Generator::classRet = templateVals[clz->templateIdMs[acc.value.value()]];
               }else Generator::classRet = clz;
               Generator::structRet = nullptr;
          }
          return Igel::setDbg(builder,builder->CreateLoad(ret.first->types[fid],ptr),(Position) acc);
     }
}

llvm::Value * NodeTermAcces::generatePointer(llvm::IRBuilder<> *builder) {
     Generator::arrRet = false;
     if(call) {
          Value* val = call->generate(builder);
          if(auto func = Generator::instance->m_file->findIgFunc(call->name->mangle(),call->params)) {
               auto clz = dynamic_cast<Class*>(func.value()->retTypeName->type);
               auto str = dynamic_cast<Struct*>(func.value()->retTypeName->type);
               if(!clz || !str)return nullptr;
               Igel::check_Pointer(builder,val);

               if(clz != nullptr)Generator::setTypeNameRet(clz);
               else Generator::setTypeNameRet(str);

               if(acc.value.value() == ARR_LENGTH && Generator::arrRet) {
                    return  builder->CreateStructGEP(clz?clz->strType:str->getStrType(builder),val,0);
               }

               if(clz) {
                    if(!Igel::SecurityManager::canAccess(clz,superType,clz->varAccesses[acc.value.value()])) {
                         Igel::err("Cannot access: " + acc.value.value(),acc);
                    }
               }
               uint fid = clz?clz->varIdMs[acc.value.value()]:str->varIdMs[acc.value.value()];
               return  builder->CreateStructGEP(clz?clz->strType:str->getStrType(builder),val,fid);

          }else return nullptr;
     }

     Value* val;
     bool isClz;

     Struct* str = nullptr;
     StructVar* var = nullptr;

     Class* clz = nullptr;
     ClassVar* clzVar = nullptr;
     if(contained) {
          Generator::contained = true;
          Generator::structRet = nullptr;
          Generator::classRet = nullptr;
          val = contained.value()->generatePointer(builder);
          if(Generator::structRet) {
               str = Generator::structRet;
               Generator::_final = str->finals[acc.value.value()];
               Generator::structRet = nullptr;
               isClz = false;
          }else {
               Generator::contained = true;
               val = contained.value()->generate(builder);
               if(!dynamic_cast<Class*>(Generator::classRet))Igel::err("This opperation can only be performed on classes",pos);
               clz = dynamic_cast<Class*>(Generator::classRet);
               Generator::_final = clz->finals[acc.value.value()];
               Generator::classRet = nullptr;
               isClz = true;
          }
          Generator::contained = false;
     }else {
          if(var = dynamic_cast<StructVar*>(Generator::instance->getVar(id->mangle()))) {
               Generator::_final = var->str->finals[acc.value.value()];
               val = var->alloc;
               isClz = false;
          }else {
               auto tst = dynamic_cast<ClassVar*>(Generator::instance->getVar(id->mangle()));
               Generator::_final = dynamic_cast<Class*>(tst->clazz)?dynamic_cast<Class*>(tst->clazz)->finals[acc.value.value()]:false;
               clzVar = dynamic_cast<ClassVar*>(Generator::instance->getVar(id->mangle()));
               val = Igel::setDbg(builder,builder->CreateLoad(clzVar->type,clzVar->alloc),pos);
               isClz = true;
          }
     }

     if(!(str || var || clz || clzVar)) {
          Igel::err("Neither contained value nor variable",acc);
     }
     Igel::check_Pointer(builder,val);

     if(!isClz) {
          //TODO add error when variable name is not contained in the struct
          uint fid = var ? var->vars[acc.value.value()]:str->varIdMs[acc.value.value()];
          if(acc.value.value() == ARR_LENGTH && Generator::arrRet)fid = 0;
          Value* ptr = builder->CreateStructGEP(var?var->strType:str->getStrType(builder),val,fid);
          Generator::setTypeNameRet(str?str:var->str);
          if(var) {
               Generator::structRet = var->str;
               return ptr;
          }

          if(str->typeName[fid] != nullptr) {
               if(const auto strT = Generator::instance->m_file->findStruct(str->typeName[fid]->mangle(),builder)) {
                    Generator::structRet = strT.value().second;
                    Generator::classRet = nullptr;
               }
               else Generator::structRet = nullptr;
          }else Generator::structRet = nullptr;
          return ptr;
     }else {
          if(!dynamic_cast<Class*>(clzVar?clzVar->clazz:clz)) {
               Igel::err("Can only access fields of classes",acc);
          }
          std::vector<PolymorphicType*> templateVals = clzVar?clzVar->templateVals:Generator::typeNameRet->templateTypes;
          if(!clz)clz = dynamic_cast<Class*>(clzVar->clazz);

          uint fid;
          Value* ptr = nullptr;
          if(!Generator::stump) {
               auto ret = clz->getVarialbe(acc.value.value());
               if(!ret.first) {
                    Igel::err("No varialbe named " + acc.value.value() + " in: " + clz->mangle(),acc);
               }
               fid = ret.second;
               if(!Igel::SecurityManager::canAccess(ret.first,superType,ret.first->varAccesses[acc.value.value()])) {
                    Igel::err("Cannot access: " + acc.value.value(),acc);
               }
               if(acc.value.value() == ARR_LENGTH && Generator::arrRet)fid = 0;
               ptr = builder->CreateStructGEP(ret.first->strType,val,ret.second);
          }
          Generator::setTypeNameRet(clz,templateVals);
          if(Generator::contained) {
               auto ret = clz->getVarialbe(acc.value.value());
               if(const auto strT = Generator::findClass(ret.first->vars[ret.second - ret.first->vTablePtrs]->typeName->mangle(),builder)) {
                    Generator::classRet = strT.value().second;
                    Generator::structRet = nullptr;
               }
               Generator::contained = false;
          }else {
               if (clz->templateIdMs.contains(acc.value.value())) {
                    Generator::classRet = templateVals[clz->templateIdMs[acc.value.value()]];
               }else Generator::classRet = clz;
               Generator::structRet = nullptr;
          }
          Generator::stump = false;
          return ptr;
     }
}

llvm::Value* NodeTermArrayAcces::generate(llvm::IRBuilder<>* builder) {
     Generator::arrRet = true;
     if(contained) {
          if(dynamic_cast<NodeTermStructAcces*>(contained.value())) {
               Struct* str = Generator::structRet;
               Generator::structRet = nullptr;
               uint fid = str->varIdMs[Igel::Mangler::mangleName(cont)];
               if(auto strRet = Generator::instance->m_file->findStruct(str->typeName[fid]->mangle(),builder)) {
                    Generator::setTypeNameRet(strRet.value().second);
                    Generator::structRet = strRet.value().second;
                    return Igel::setDbg(builder,builder->CreateLoad(str->types[fid],generatePointer(builder)),pos);
               }
               return nullptr;
          }
     }
     auto* var = static_cast<ArrayVar*>(Generator::instance->getVar(Igel::Mangler::mangleName(cont)));
     _signed = var->_signed;
     return Igel::setDbg(builder,builder->CreateLoad(var->size == exprs.size()?var->type:builder->getPtrTy(),generatePointer(builder)),pos);
}

llvm::Value* NodeTermArrayAcces::generatePointer(llvm::IRBuilder<>* builder) {
     Value* ptr = nullptr;
     ArrayVar* var = nullptr;
     if(contained) {
          ptr = contained.value()->generatePointer(builder);
     }else {
          if(var = dynamic_cast<ArrayVar*>(Generator::instance->getVar(Igel::Mangler::mangleName(cont)))) {
               _signed = var->_signed;
               if(var->typeName.has_value()) {
                    if(auto strRet = Generator::instance->m_file->findStruct(var->typeName.value()->mangle(),builder)) {
                         Generator::structRet = strRet.value().second;
                         Generator::classRet = nullptr;
                    }
                    else if(auto clazzRet = Generator::findClass(var->typeName.value()->mangle(),builder)) {
                         Generator::classRet = clazzRet.value().second;
                         Generator::structRet = nullptr;
                    }
               }
          }
     }
     if(!ptr && !var) {
          Igel::err("Neither contained value nor variable",pos);
     }
     Igel::check_Pointer(builder,ptr);
     uint i = 0;

     Type* ty;
     if(llvm::AllocaInst::classof(var->alloc))ty = static_cast<AllocaInst*>(var->alloc)->getAllocatedType();
     else if(GlobalVariable::classof(var->alloc))ty = static_cast<GlobalVariable*>(var->alloc)->getValueType();
     Igel::check_Pointer(builder,builder->CreateLoad(var->alloc->getType(),var->alloc));
     while (i < exprs.size() - 1 && exprs.size() >= 1) {
          Value* val = Igel::setDbg(builder,builder->CreateLoad(ptr?ptr->getType():ty,ptr?ptr:var->alloc),pos);
          Value* idx = builder->CreateAdd(builder->getInt64(1),exprs[i]->generate(builder));
          idx = builder->CreateMul(idx,builder->getInt64(8));
          ptr = builder->CreateInBoundsGEP(builder->getInt8Ty(),val,idx);
          i++;
     }

     Value* val = Igel::setDbg(builder,builder->CreateLoad(ptr?ptr->getType():ty,ptr?ptr:var->alloc),pos);
     Value* expr = exprs[i]->generate(builder);
     Value* idx = builder->CreateMul(builder->getInt64(Generator::instance->m_module->getDataLayout().getTypeSizeInBits(expr->getType()) / 8),expr);
     idx = builder->CreateAdd(builder->getInt64(8),idx);
     Igel::check_Arr(builder,val,expr);
     ptr = builder->CreateInBoundsGEP(builder->getInt8Ty(),val,idx);

     return ptr;
}

llvm::Value* NodeTermArrayLength::generate(llvm::IRBuilder<>* builder) {
     arr->exprs.pop_back();
     Value* val = arr->generate(builder);
     val = builder->CreateInBoundsGEP(val->getType(),val,builder->getInt64(0));
     return Igel::setDbg(builder,builder->CreateLoad(builder->getInt64Ty(),val),pos);
}

llvm::Value* NodeTermArrayLength::generatePointer(llvm::IRBuilder<>* builder) {
     arr->exprs.pop_back();
     Value* val = arr->generate(builder);
     return builder->CreateInBoundsGEP(val->getType(),val,builder->getInt64(0));
}

llvm::Value* NodeTermStructAcces::generate(llvm::IRBuilder<>* builder) {
     if(call) {
          call->generate(builder);
          if(auto func = Generator::instance->m_file->findIgFunc(call->name->mangle(),call->params)) {
               auto str = dynamic_cast<Struct*>(func.value()->retTypeName->type);
               if(!str)return nullptr;
               Generator::setTypeNameRet(str);
               auto it = std::ranges::find(str->varIds,acc.value.value());
               if(it == str->varIds.end())return nullptr;
               uint fid = it - str->varIds.begin();
               return Igel::setDbg(builder,builder->CreateLoad(str->types[fid],this->generatePointer(builder)),(Position) acc);
          }else return nullptr;
     }

     Value* val;
     Struct* str = nullptr;
     StructVar* var = nullptr;
     if(contained) {
          val = contained.value()->generate(builder);
          str = Generator::structRet;
          Generator::_final = str->finals[acc.value.value()];
          Generator::structRet = nullptr;
     }else {
          var = dynamic_cast<StructVar*>(Generator::instance->getVar(id->mangle()));
          Generator::_final = var->str->finals[acc.value.value()];;
          val = var->alloc;
     }
     if(!str && !var) {
          Igel::err("Neither contained value nor variable",acc);
     }

     const uint fid = var ? var->vars[acc.value.value()]:str->varIdMs[acc.value.value()];
     Value* ptr = builder->CreateStructGEP(var?var->strType:str->getStrType(builder),val,fid);
     Generator::setTypeNameRet(str?str:var->str);
     if(var) {
          Generator::structRet = var->str;
          Generator::classRet = nullptr;
          return Igel::setDbg(builder,builder->CreateLoad(var->types[fid],ptr),acc);
     }

     if(str->typeName[fid] != nullptr) {
          if(const auto strT = Generator::instance->m_file->findStruct(str->typeName[fid]->mangle(),builder)) {
               Generator::structRet = strT.value().second;
               Generator::classRet = nullptr;
          }
          else Generator::structRet = nullptr;
     }else Generator::structRet = nullptr;
     return ptr;
}

llvm::Value* NodeTermStructAcces::generatePointer(llvm::IRBuilder<>* builder) {
     if(call) {
          Value* val = call->generate(builder);
          if(auto func = Generator::instance->m_file->findIgFunc(call->name->mangle(),call->params)) {
               auto str = dynamic_cast<Struct*>(func.value()->retTypeName->type);
               if(!str)return nullptr;
               Generator::setTypeNameRet(str);
               auto it = std::ranges::find(str->varIds,acc.value.value());
               if(it == str->varIds.end())return nullptr;
               uint fid = it - str->varIds.begin();
               return builder->CreateStructGEP(str->getStrType(builder),val,fid);

          }else return nullptr;
     }
     Value* val;
     Struct* str = nullptr;
     StructVar* var = nullptr;
     if(contained) {
          val = contained.value()->generatePointer(builder);
          str = Generator::structRet;
          Generator::_final = str->finals[acc.value.value()];
          Generator::structRet = nullptr;
     }else {
          var = dynamic_cast<StructVar*>(Generator::instance->getVar(id->mangle()));
          Generator::_final = var->str->finals[acc.value.value()];
          val = var->alloc;
     }

     if(!str && !var) {
          Igel::err("Neither contained value nor variable",acc);
     }

     //TODO add error when variable name is not contained in the struct
     const uint fid = var ? var->vars[acc.value.value()]:str->varIdMs[acc.value.value()];
     Value* ptr = builder->CreateStructGEP(var?var->strType:str->getStrType(builder),val,fid);
     Generator::setTypeNameRet(str?str:var->str);
     if(var) {
          Generator::structRet = var->str;
          Generator::classRet = nullptr;
          return ptr;
     }

     if(str->typeName[fid] != nullptr) {
          if(const auto strT = Generator::instance->m_file->findStruct(str->typeName[fid]->mangle(),builder)) {
               Generator::structRet = strT.value().second;
               Generator::classRet = nullptr;
          }
          else Generator::structRet = nullptr;
     }else Generator::structRet = nullptr;
     return ptr;
}

/*llvm::Value* NodeTermClassAcces::generate(llvm::IRBuilder<>* builder) {
     if(call) {
          call->generate(builder);
          if(auto func = Generator::instance->m_file->findIgFunc(call->name->mangle(),call->params)) {
               auto str = dynamic_cast<Class*>(func.value()->retTypeName->type);
               if(!str)return nullptr;
               Generator::setTypeNameRet(str);
               uint fid = str->varIdMs[acc.value.value()];
               return Igel::setDbg(builder,builder->CreateLoad(str->types[fid],this->generatePointer(builder)),(Position) acc);
          }else return nullptr;
     }

     Value* val;
     Class* str = nullptr;
     ClassVar* var = nullptr;
     if(contained) {
          val = contained.value()->generate(builder);
          if(!dynamic_cast<Class*>(Generator::classRet))Igel::err("This opperation can only be performed on classes",pos);
          str = dynamic_cast<Class*>(Generator::classRet);
          Generator::_final = str->finals[acc.value.value()];
     }else {
          var = dynamic_cast<ClassVar*>(Generator::instance->getVar(id->mangle()));
          Generator::_final = dynamic_cast<Class*>(var->clazz)?dynamic_cast<Class*>(var->clazz)->finals[acc.value.value()]:false;
          val = Igel::setDbg(builder,builder->CreateLoad(var->type,var->alloc),pos);
     }
     if(!str && !var) {
          Igel::err("Neither contained value nor variable",acc);
     }
     Igel::check_Pointer(builder,val);

     const uint fid = var ? var->vars[acc.value.value()]:str->varIdMs[acc.value.value()];
     Value* ptr = builder->CreateStructGEP(var?var->strType:str->strType,val,fid);
     Generator::setTypeNameRet(str?str:var->clazz);
     if(var) {
          Generator::classRet = dynamic_cast<Class*>(var->clazz);
          Generator::structRet = nullptr;
          return Igel::setDbg(builder,builder->CreateLoad(var->types[fid],ptr),acc);
     }

     if(str->typeName[fid - 1] != nullptr) {
          if(const auto strT = Generator::findClass(str->typeName[fid - 1]->mangle(),builder)) {
               Generator::classRet = strT.value().second;
               Generator::structRet = nullptr;
          }
          else Generator::classRet = nullptr;
     }else Generator::classRet = nullptr;
     return Igel::setDbg(builder,builder->CreateLoad(str->types[fid - 1],ptr),(Position) acc);
}

llvm::Value* NodeTermClassAcces::generatePointer(llvm::IRBuilder<>* builder) {
     if(call) {
          Value* val = call->generate(builder);
          if(auto func = Generator::instance->m_file->findIgFunc(call->name->mangle(),call->params)) {
               auto str = dynamic_cast<Class*>(func.value()->retTypeName->type);
               if(!str)return nullptr;
               Igel::check_Pointer(builder,val);
               Generator::setTypeNameRet(str);
               uint fid = str->varIdMs[acc.value.value()];
               return builder->CreateStructGEP(str->strType,val,fid);

          }else return nullptr;
     }
     Value* val;
     Class* str = nullptr;
     ClassVar* var = nullptr;
     if(contained) {
          val = contained.value()->generate(builder);
          if(!dynamic_cast<Class*>(Generator::classRet))Igel::err("This opperation can only be performed on classes",pos);
          str = dynamic_cast<Class*>(Generator::classRet);
          Generator::_final = str->finals[acc.value.value()];
     }else {
          var = dynamic_cast<ClassVar*>(Generator::instance->getVar(id->mangle()));
          Generator::_final = dynamic_cast<Class*>(var->clazz)?dynamic_cast<Class*>(var->clazz)->finals[acc.value.value()]:false;
          val = Igel::setDbg(builder,builder->CreateLoad(var->type,var->alloc),acc);
     }

     if(!str && !var) {
          Igel::err("Neither contained value nor variable",acc);
     }
     Igel::check_Pointer(builder,val);

     //TODO add error when variable name is not contained in the struct
     const uint fid = var ? var->vars[acc.value.value()]:str->varIdMs[acc.value.value()];
     Value* ptr = builder->CreateStructGEP(var?var->strType:str->strType,val,fid);
     Generator::setTypeNameRet(str?str:var->clazz);
     if(var) {
          Generator::classRet = dynamic_cast<Class*>(var->clazz);
          Generator::structRet = nullptr;
          return ptr;
     }

     if(str->typeName[fid - 1] != nullptr) {
          if(const auto strT = Generator::findClass(str->typeName[fid - 1]->mangle(),builder)) {
               Generator::classRet = strT.value().second;
               Generator::structRet = nullptr;
          }
          else Generator::classRet = nullptr;
     }else Generator::classRet = nullptr;
     return ptr;
}*/

llvm::Value* NodeTermAcces::generateClassPointer(llvm::IRBuilder<>* builder) {
     if(call) {
          Value* val = call->generate(builder);
          if(auto func = Generator::instance->m_file->findIgFunc(call->name->mangle(),call->params)) {
               auto str = dynamic_cast<Class*>(func.value()->retTypeName->type);;
               if(!str)return nullptr;
               Igel::check_Pointer(builder,val);
               Generator::setTypeNameRet(str);
               uint fid = str->varIdMs[acc.value.value()];
               return  val;
          }
     }

     Value* val;
     Class* str = nullptr;
     ClassVar* var = nullptr;
     if(contained) {
          val = contained.value()->generatePointer(builder);
          if(!dynamic_cast<Class*>(Generator::classRet))Igel::err("This opperation can only be performed on classes",pos);
          str = dynamic_cast<Class*>(Generator::classRet);
          Generator::_final = str->finals[acc.value.value()];
     }else {
          var = dynamic_cast<ClassVar*>(Generator::instance->getVar(id->mangle()));
          Generator::_final = var->_final;
          val = Igel::setDbg(builder,builder->CreateLoad(var->type,var->alloc),acc);
     }

     if(!str && !var) {
          Igel::err("Neither contained value nor variable",acc);
     }
     Igel::check_Pointer(builder,val);

     //TODO add error when variable name is not contained in the struct
     const uint fid = var ? var->vars[acc.value.value()]:str->varIdMs[acc.value.value()];
     Generator::setTypeNameRet(str?str:var->clazz);
     if(var) {
          Generator::classRet = dynamic_cast<Class*>(var->clazz);
          Generator::setTypeNameRet(var->clazz,var->templateVals);
          Generator::structRet = nullptr;
          return val;
     }

     if(str->typeName[fid] != nullptr) {
          if(const auto strT = Generator::findClass(str->typeName[fid]->mangle(),builder)) {
               Generator::classRet = strT.value().second;
               Generator::setTypeNameRet(strT.value().second);
               Generator::structRet = nullptr;
          }
          else Generator::classRet = nullptr;
     }else Generator::classRet = nullptr;
     return val;
}

llvm::Value * NodeStmtSuperConstructorCall::generate(llvm::IRBuilder<> *builder) {
     if(!_this->extending.has_value()) {
          Igel::err("Call to super constructor can only be done in class with super at: " + _this->mangle(),pos);
     }
     std::vector<Type*> types {builder->getPtrTy()};
     std::vector<BeContained*> names {new Name("")};
     std::vector<bool> sing = {true};

     std::vector<Value*> params {};
     params.reserve(exprs.size());

     for (auto expr : exprs) {
          auto exprRet = expr->generate(builder);
          params.push_back(exprRet);
          Generator::clearTypeNameRet();
          types.push_back(exprRet->getType());
          names.push_back(Generator::typeNameRet->type);
          sing.push_back(expr->_signed);
          Generator::clearTypeNameRet();
     }

     FunctionType* type = FunctionType::get(PointerType::get(builder->getContext(),0),{Type::getInt64Ty(builder->getContext())},false);
     ArrayRef<Type*> llvmTypes {types};
     auto calle = Generator::instance->m_module->getOrInsertFunction(Igel::Mangler::mangle(_this->extending.value(),types,names,sing,true),builder->getVoidTy(),llvmTypes);

     Var* ptrA = Generator::instance->getVar("_Z4this");
     Value* ptrL = Igel::setDbg(builder,builder->CreateLoad(ptrA->getType(),ptrA->alloc),pos);;
     params.insert(params.begin(),ptrL);
     //for (auto expr : exprs) params.push_back(expr->generate(builder));

     Value* ptr = builder->CreateCall(calle,params);
     Generator::setTypeNameRet(_this->extending.value());
     return ptr;
}

llvm::Value* NodeStmtLet::generate(llvm::IRBuilder<>* builder) {
     if(_static) {
          GlobalValue* val = new GlobalVariable(*Generator::instance->m_module,type?type:builder->getPtrTy(),final,GlobalValue::ExternalLinkage,
               type?ConstantInt::get(type,0):ConstantPointerNull::get(PointerType::get(*Generator::instance->m_contxt,0)),mangle());
          std::pair<llvm::Value*,Var*> var = generateImpl(builder,true);
          var.second->alloc = val;
          Generator::instance->createStaticVar(mangle(),var.first,var.second);
          return val;
     }
     std::pair<llvm::Value*,Var*> var = generateImpl(builder,true);
     Generator::instance->createVar(mangle(),var.second);
     if(Generator::instance->debug) {
          Debug dbg = Generator::instance->getDebug();
          DIScope* scope = Generator::instance->dbgScopes.empty()?dbg.file:Generator::instance->dbgScopes.back();
          DIType* type = nullptr;
          if(typeName && typeName->dbgType) {
               if(typeName->dbgpointerType)type = typeName->dbgpointerType;
               else type = typeName->dbgType;
          }
          else type = dbg.builder->createBasicType(name,Generator::instance->m_module->getDataLayout().getTypeSizeInBits(var.second->getType()),Generator::getEncodingOfType(var.second->getType()));
          auto dbgVar = dbg.builder->createAutoVariable(scope,name,dbg.file,pos.line,type);
          dbg.builder->insertDeclare(var.second->alloc,dbgVar,dbg.builder->createExpression(),DILocation::get(builder->getContext(),pos.line,pos._char,scope),builder->GetInsertBlock());
     }
     return var.first;
}

llvm::Value* NodeStmtLet::generatePointer(llvm::IRBuilder<>* builder) {
     if(_static) {
          GlobalValue* val = new GlobalVariable(*Generator::instance->m_module,type?type:builder->getPtrTy(),final,GlobalValue::ExternalLinkage,
               type?ConstantInt::get(type,0):ConstantPointerNull::get(PointerType::get(*Generator::instance->m_contxt,0)),mangle());
          std::pair<llvm::Value*,Var*> var = generateImpl(builder,false);
          var.second->alloc = val;
          Generator::instance->createStaticVar(mangle(),var.first,var.second);
          return val;
     }
     std::pair<llvm::Value*,Var*> var = generateImpl(builder,false);
     return var.first;
}

std::pair<llvm::Value*, Var*> NodeStmtPirimitiv::generateImpl(llvm::IRBuilder<>* builder,bool full) {
     Value* val = expr.has_value()?expr.value()->generate(builder) : ConstantInt::get(Generator::getType(static_cast<TokenType>(sid)),0);

     AllocaInst* alloc = nullptr;
     if(builder->GetInsertBlock() && full) {
          alloc = builder->CreateAlloca(type);
          Igel::setDbg(builder,builder->CreateStore(val,alloc),pos);
     }
     return {val,new Var{alloc,_signed,final}};
}

std::string NodeStmtPirimitiv::mangle() {
     return Igel::Mangler::mangleName(this);
}

std::string NodeStmtNew::mangle() {
     return Igel::Mangler::mangle(this->name);
}

std::string NodeStmtArr::mangle() {
     return Igel::Mangler::mangleName(this);
}

std::string NodeStmtEnum::mangle() {
     return Igel::Mangler::mangle(this->name);
}

std::pair<llvm::Value *, Var *> NodeStmtEnum::generateImpl(llvm::IRBuilder<> *builder,bool full) {
     Value* val = ConstantInt::get(builder->getInt32Ty(),id);

     AllocaInst* alloc = nullptr;
     if(builder->GetInsertBlock() && full) {
          alloc = builder->CreateAlloca(builder->getInt32Ty());
          Igel::setDbg(builder,builder->CreateStore(val,alloc),pos);
     }
     return {val,new Var{alloc,false,final}};
}

std::pair<llvm::Value*, Var*> NodeStmtStructNew::generateImpl(llvm::IRBuilder<>* builder,bool full) {
     if(auto structT = Generator::instance->m_file->findStruct(typeName->mangle(),builder)) {
          AllocaInst* alloc = builder->CreateAlloca(structT.value().first);

          auto var = new StructVar(alloc,final);
          var->strType = structT.value().first;
          var->str = structT.value().second;
          var->types = structT.value().second->types;
          var->vars = structT.value().second->varIdMs;

          std::unordered_map<std::string,bool> finals;
          for(size_t i = 0;i < structT.value().second->vars.size();i++) {
               Value* val = builder->CreateStructGEP(structT.value().first,alloc,i);
               Value* gen = structT.value().second->vars[i]->generate(builder);
               Igel::setDbg(builder,builder->CreateStore(gen,val),pos);
          }
          return {alloc,var};
     }else {
          Igel::err("Undeclarde Type " + typeName->mangle(),pos);
     }
}

std::pair<llvm::Value*, Var*> NodeStmtClassNew::generateImpl(llvm::IRBuilder<>* builder,bool full) {
     Generator::clearTypeNameRet();
     Value* gen = term->generate(builder);
     if(!Generator::typeNameRet) {
          if(!dynamic_cast<ContainableType*>(typeName))Igel::internalErr("Class new of non-containable type");
          auto type = PointerType::get(builder->getContext(),0);
          AllocaInst* alloc = nullptr;
          if(builder->GetInsertBlock() && full) {
               alloc = builder->CreateAlloca(type);
               builder->CreateStore(gen,alloc);
          }

          auto var = new ClassVar(alloc,final);
          var->templateVals = templateVals;
          var->type = builder->getPtrTy();
          var->strType = nullptr;
          var->clazz = dynamic_cast<ContainableType*>(typeName);
          var->types = {builder->getPtrTy()};
          std::vector<IgFunction*> typeFuncs = dynamic_cast<Class*>(typeName)?dynamic_cast<Class*>(typeName)->funcs:dynamic_cast<Interface*>(typeName)->funcs;
          for (uint i = 0; i < typeFuncs.size();i++)var->funcs[typeFuncs[i]->name] = i;
          Generator::clearTypeNameRet();
          return {gen,var};
     }

     if(Generator::typeNameRet->templateTypes != templateVals && !Generator::typeNameRet->templateTypes.empty())Igel::err("Template types do not match",pos);
     if(Generator::typeNameRet->type != typeName && !dynamic_cast<NodeTermNull*>(term)) {
          Igel::err("Cannot assign a value of type " + (Generator::typeNameRet->type?Generator::typeNameRet->type->mangle():"null") + " to a variable of type " + typeName->mangle(),pos);
     }
     if(auto clazz = /*Generator::findClass(typeName->mangle(),builder)*/dynamic_cast<Class*>(typeName)) {
          auto type = PointerType::get(builder->getContext(),0);
          AllocaInst* alloc = nullptr;
          if(builder->GetInsertBlock() && full) {
               alloc = builder->CreateAlloca(type);
               builder->CreateStore(gen,alloc);
          }

          auto var = new ClassVar(alloc,final);
          var->templateVals = templateVals;
          var->type = builder->getPtrTy();
          var->strType = clazz->strType;
          var->clazz = clazz;
          var->types = clazz->types;
          for(size_t i = 0;i < clazz->vars.size();i++) {
               NodeStmtLet* let = clazz->vars[i];
               std::string str = clazz->vars[i]->name;
               if(auto pirim = dynamic_cast<NodeStmtPirimitiv*>(let))var->signage.push_back(pirim->sid <= (char) TokenType::_long);
               else var->signage.push_back(false);
               var->vars[str] = i + 1;
          }
          for (uint i = 0; i < clazz->funcs.size();i++)var->funcs[clazz->funcs[i]->name] = i;
          Generator::clearTypeNameRet();
          return {gen,var};
     }else if(auto intf = /*Generator::findInterface(typeName->mangle())*/dynamic_cast<Interface*>(typeName)) {
          auto type = PointerType::get(builder->getContext(),0);
          AllocaInst* alloc = nullptr;
          if(builder->GetInsertBlock() && full) {
               alloc = builder->CreateAlloca(type);
               builder->CreateStore(gen,alloc);
          }

          auto var = new ClassVar(alloc,final);
          var->templateVals = templateVals;
          var->type = builder->getPtrTy();
          var->strType = nullptr;
          var->clazz = intf;
          var->types = {builder->getPtrTy()};
          for (uint i = 0; i < intf->funcs.size();i++)var->funcs[intf->funcs[i]->name] = i;
          Generator::clearTypeNameRet();
          return {gen,var};
     }else {
          Generator::clearTypeNameRet();
          Igel::err("Undeclarde Type: " + typeName->mangle(),pos);
     }
}

Value* createArr(std::vector<Value*> sizes,std::vector<Type*> types,uint i,llvm::IRBuilder<>* builder) {
     if(i == 0)return ConstantInt::get(types[i],0);
     const llvm::FunctionCallee _new = Generator::instance->m_module->getOrInsertFunction("_Znam",PointerType::get(builder->getContext(),0),IntegerType::getInt64Ty(builder->getContext()));
     Value* var = builder->CreateCall(_new,builder->CreateMul(sizes[i - 1],ConstantInt::get(IntegerType::getInt64Ty(builder->getContext()),12)));

     return var;
}

std::pair<llvm::Value*, Var*> NodeStmtArr::generateImpl(llvm::IRBuilder<>* builder,bool full) {
     Value* val = llvm::ConstantPointerNull::get(llvm::PointerType::get(builder->getContext(),0));
     if(term)val = term->generate(builder);
     auto* var = new ArrayVar(builder->CreateAlloca(builder->getPtrTy()),sid <= 3,final,typeName != nullptr?typeName:(std::optional<BeContained*>) {});
     var->size = size;
     var->type = getType(static_cast<TokenType>(sid));
     builder->CreateStore(val,var->alloc);
     return {val,var};
}

llvm::Value* NodeStmtExit::generate(llvm::IRBuilder<>* builder) {
     Value* val = expr->generate(builder);
     Generator* gen = Generator::instance;

     return gen->exitG(val);
}

llvm::Value* NodeStmtScope::generate(llvm::IRBuilder<>* builder) {
     llvm::Value* val = nullptr;
     Generator::lastUnreachable = false;
     Generator::instance->m_vars.emplace_back();
     for (int i = startIndex;i < stmts.size();i++) {
          if(Generator::lastUnreachable)break;
          val = stmts[i]->generate(builder);
     }
     Generator::instance->m_vars.pop_back();
     return val;
};

llvm::Value* NodeStmtIf::generate(llvm::IRBuilder<>* builder){
     llvm::Value* val = expr[0]->generate(builder);
     if(val->getType() != builder->getInt1Ty())val = builder->CreateCmp(CmpInst::ICMP_NE,val,ConstantPointerNull::get(builder->getPtrTy()));
     for (size_t i = 1;i < expr.size();i++) {
          Value * res = expr[i]->generate(builder);
          if(res->getType() != builder->getInt1Ty())res = builder->CreateCmp(CmpInst::ICMP_EQ,res,ConstantInt::get(res->getType(),0));
          if(exprCond[i - 1] == TokenType::_and)val = builder->CreateLogicalAnd(val,res);
          else val = builder->CreateLogicalOr(val,res);
     }

     if(!Generator::instance->next.empty()) {
          std::cout << Generator::instance->next.back()->getName().str() << "\n";
     }
     llvm::Function* parFunc = builder->GetInsertBlock()->getParent();
     llvm::BasicBlock* then = llvm::BasicBlock::Create(builder->getContext(),"then",parFunc);
     llvm::BasicBlock* elseB = llvm::BasicBlock::Create(builder->getContext(),"else");
     llvm::BasicBlock* merge = llvm::BasicBlock::Create(builder->getContext(),"ifcont");

     builder->CreateCondBr(val,then,elseB);

     builder->SetInsertPoint(then);
     generateS(scope,builder);
     if(!Generator::lastUnreachable) {
          builder->CreateBr(merge);
     }
     then = builder->GetInsertBlock();

     if(scope_else) {
          parFunc->insert(parFunc->end(),elseB);
          builder->SetInsertPoint(elseB);
          generateS(scope_else.value(),builder);
          if(!Generator::lastUnreachable) {
               builder->CreateBr(merge);
          }
          elseB = builder->GetInsertBlock();
     }else if(else_if) {
          parFunc->insert(parFunc->end(),elseB);
          builder->SetInsertPoint(elseB);
          else_if.value()->generate(builder);
          if(!Generator::lastUnreachable) {
               builder->CreateBr(merge);
          }
          elseB = builder->GetInsertBlock();
     }else {
          parFunc->insert(parFunc->end(),elseB);
          builder->SetInsertPoint(elseB);
          builder->CreateBr(merge);
          elseB = builder->GetInsertBlock();
     }
     parFunc->insert(parFunc->end(),merge);
     builder->SetInsertPoint(merge);
     Generator::lastUnreachable = false;
     return elseB != nullptr?elseB:then;
}

llvm::Value* NodeStmtFor::generate(llvm::IRBuilder<>* builder) {
     Value* var = let->generate(builder);
     Function* func = builder->GetInsertBlock()->getParent();
     BasicBlock* head = BasicBlock::Create(builder->getContext(), "head", func);
     BasicBlock* scopeB = BasicBlock::Create(builder->getContext(), "scope",func);
     BasicBlock* tail = BasicBlock::Create(builder->getContext(), "tail");
     BasicBlock* next = BasicBlock::Create(builder->getContext(), "next",func);
     builder->CreateBr(head);
     builder->SetInsertPoint(head);

     llvm::Value* val = expr[0]->generate(builder);
     for (size_t i = 1;i < expr.size();i++) {
          if(exprCond[i - 1] == TokenType::_and)val = builder->CreateLogicalAnd(val,expr[i]->generate(builder));
          else val = builder->CreateLogicalOr(val,expr[i]->generate(builder));
     }

     builder->CreateCondBr(val,scopeB,next);
     builder->SetInsertPoint(scopeB);

     Generator::instance->after.push_back(next);
     Generator::instance->next.push_back(tail);
     generateS(scope,builder);
     Generator::instance->next.pop_back();
     Generator::instance->after.pop_back();

     builder->CreateBr(tail);
     func->insert(func->end(),tail);
     builder->SetInsertPoint(tail);
     res->generate(builder);
     builder->CreateBr(head);

     builder->SetInsertPoint(next);
     Generator::lastUnreachable = false;
     return next;
}

llvm::Value* NodeStmtWhile::generate(llvm::IRBuilder<>* builder) {
     Function* func = builder->GetInsertBlock()->getParent();
     BasicBlock* head = BasicBlock::Create(builder->getContext(), "head", func);
     BasicBlock* scopeB = BasicBlock::Create(builder->getContext(), "scope", func);
     BasicBlock* next = BasicBlock::Create(builder->getContext(), "next",func);
     builder->CreateBr(head);
     builder->SetInsertPoint(head);

     llvm::Value* val = expr[0]->generate(builder);
     for (size_t i = 1;i < expr.size();i++) {
          if(exprCond[i - 1] == TokenType::_and)val = builder->CreateLogicalAnd(val,expr[i]->generate(builder));
          else val = builder->CreateLogicalOr(val,expr[i]->generate(builder));
     }

     builder->CreateCondBr(val,scopeB,next);
     builder->SetInsertPoint(scopeB);

     Generator::instance->after.push_back(next);
     Generator::instance->next.push_back(head);
     generateS(scope,builder);
     Generator::instance->next.pop_back();
     Generator::instance->after.pop_back();

     builder->CreateBr(head);
     next->moveAfter(&func->back());
     builder->SetInsertPoint(next);
     Generator::lastUnreachable = false;
     return next;
}

BasicBlock* NodeStmtCase::generate(llvm::IRBuilder<>* builder) {
     Function* func = builder->GetInsertBlock()->getParent();
     BasicBlock* _case = BasicBlock::Create(builder->getContext(),"case",func);
     if(_default)Generator::instance->_switch.back()->setDefaultDest(_case);
     else
          Generator::instance->_switch.back()->addCase(static_cast<ConstantInt*>(cond->generate(builder)),_case);
     builder->SetInsertPoint(_case);
     return _case;
}

llvm::Value* NodeStmtSwitch::generate(llvm::IRBuilder<>* builder) {
     Function* func = builder->GetInsertBlock()->getParent();
     BasicBlock* next = BasicBlock::Create(builder->getContext(),"next",func);
     SwitchInst* inst = builder->CreateSwitch(cond->generate(builder),next);
     Generator::instance->_switch.push_back(inst);
     Generator::instance->after.push_back(next);
     generateS(scope,builder);
     Generator::instance->_switch.pop_back();
     Generator::instance->after.pop_back();
     next->moveAfter(&func->back());
     builder->SetInsertPoint(next);
     return inst;
}


llvm::Value* NodeStmtReassign::generate(llvm::IRBuilder<>* builder) {
     Generator::_final = false;
     Value* val = id->generatePointer(builder);
     std::string name;
     if(dynamic_cast<NodeTermAcces*>(id))name = dynamic_cast<NodeTermAcces*>(id)->acc.value.value();
     else name = dynamic_cast<NodeTermId*>(id)->cont->mangle();
     if(Generator::_final) {
          Igel::err("Cannot modify final variable: " + name,pos);
     }
     if(op == TokenType::eq) {
          if(Generator::instance->debug) {
               Debug dbg = Generator::instance->getDebug();
               DILocalVariable* var = dbg.builder->createAutoVariable(Generator::instance->getScope(),name,dbg.file,pos.line,
                    dbg.builder->createBasicType(name,
                         Generator::instance->m_module->getDataLayout().getTypeSizeInBits(val->getType()),Generator::getEncodingOfType(val->getType())));
               dbg.builder->insertDbgValueIntrinsic(val,var,dbg.builder->createExpression(),DILocation::get(builder->getContext(),pos.line,pos._char,Generator::instance->getScope()),builder->GetInsertBlock());
          }
          return Igel::setDbg(builder,builder->CreateStore(expr->generate(builder),val),pos);
     }else {
          Value* load = id->generate(builder);
          Value* _new = nullptr;
          if(expr != nullptr)_new = expr->generate(builder);
          Value* res = generateReassing(load,_new,op,builder);
          StoreInst* inst = builder->CreateStore(res,val);
          if(Generator::instance->debug) {
               Debug dbg = Generator::instance->getDebug();
               DILocalVariable* var = dbg.builder->createAutoVariable(Generator::instance->getScope(),name,dbg.file,pos.line,
                    dbg.builder->createBasicType(name,
                         Generator::instance->m_module->getDataLayout().getTypeSizeInBits(val->getType()),Generator::getEncodingOfType(val->getType())));
               dbg.builder->insertDbgValueIntrinsic(val,var,dbg.builder->createExpression(),DILocation::get(builder->getContext(),pos.line,pos._char,Generator::instance->getScope()),builder->GetInsertBlock());
          }
          return Igel::setDbg(builder,inst,pos);
     }
}

llvm::Value* NodeStmtArrReassign::generate(llvm::IRBuilder<>* builder) {
     //TODO add exception throwing when IndexOutOfBounds
     Generator::_final = false;
     Value* ep = acces->generatePointer(builder);
     if(Generator::_final) {
          Igel::err("Cannot modify final variable ",pos);
     }
     if(op == TokenType::eq) {
          Value* gen = expr.value()->generate(builder);
          Value* val = builder->CreateStore(gen,ep);
          return val;
     }else {
          auto* var = static_cast<ArrayVar*>(Generator::instance->getVar(Igel::Mangler::mangleName(acces->cont)));
          Value* load = Igel::setDbg(builder,builder->CreateLoad(var->type,generatePointer(builder)),pos);
          Value* _new = expr.value()->generate(builder);
          Value* res = generateReassing(load,_new,op,builder);
          return Igel::setDbg(builder,builder->CreateStore(res,ep),pos);
     }
};

llvm::Value* NodeStmtReturn::generate(llvm::IRBuilder<>* builder)  {
     Generator::lastUnreachable = true;
     if(val) {
          return Igel::setDbg(builder,builder->CreateRet(val.value()->generate(builder)),pos);
     }

     //TODO add pos for close curly bracket
     return  builder->CreateRetVoid();
};

llvm::Value* NodeStmtBreak::generate(llvm::IRBuilder<>* builder) {
     Value* val = Igel::setDbg(builder,builder->CreateBr(Generator::instance->after.back()),pos);
     Generator::lastUnreachable = true;
     return val;
}

llvm::Value* NodeStmtContinue::generate(llvm::IRBuilder<>* builder) {
     Value* val = Igel::setDbg(builder,builder->CreateBr(Generator::instance->next.back()),pos);
     Generator::lastUnreachable = true;
     return val;
}

llvm::Value * NodeStmtThrow::generate(llvm::IRBuilder<> *builder) {
     llvm::FunctionCallee cxa_Throw = Generator::instance->m_module->getOrInsertFunction("__cxa_throw",FunctionType::get(builder->getVoidTy(),
          {builder->getPtrTy(),builder->getPtrTy(),builder->getPtrTy()},false));
     llvm::FunctionCallee cxa_alloc = Generator::instance->m_module->getOrInsertFunction("__cxa_allocate_exception",FunctionType::get(builder->getPtrTy(),builder->getInt64Ty(),false));

     Generator::clearTypeNameRet();
     Value* val = expr->generatePointer(builder);
     std::optional<std::pair<StructType*,Class*>> clazz = Generator::findClass(Generator::typeNameRet->type->mangle(),builder);
     if(!clazz.has_value()) {
          Igel::err("Could not find class: " + Generator::typeNameRet->type->mangle(),pos);
     }
     if(!Generator::instance->unreach) {
          Generator::instance->unreach = BasicBlock::Create(builder->getContext(),"unreachable",builder->GetInsertBlock()->getParent());
     }
     Generator::lastUnreachable = true;
     Value* excep = builder->CreateCall(cxa_alloc,builder->getInt64(8));
     builder->CreateStore(val,excep);

     if(auto clazzExc = Generator::findClass("Exception",builder)) {
          if(clazz.value().second->isSubTypeOf(clazzExc.value().second)) {
               Value* loaded = expr->generate(builder);
               Value* excepCast = Igel::stat_Cast(builder,clazzExc.value().second,clazz.value().second,loaded);

               Value* load = Igel::setDbg(builder,builder->CreateLoad(val->getType(),val),pos);
               Value* load1 = Igel::setDbg(builder,builder->CreateLoad(val->getType(),load),pos);
               Value* load2 = Igel::setDbg(builder,builder->CreateLoad(val->getType(),load1),pos);
               builder->CreateCall(FunctionType::get(builder->getVoidTy(),{builder->getPtrTy()},false),load2,{excepCast});
          }
     }

     Generator::clearTypeNameRet();
     if(Generator::catches.empty()) {
          Igel::setDbg(builder,builder->CreateCall(cxa_Throw,{excep,clazz.value().second->getClassInfos(builder).typeInfoPointVar,ConstantPointerNull::get(builder->getPtrTy())}),pos);
          return  builder->CreateUnreachable();
     }else return Igel::setDbg(builder,builder->CreateInvoke(cxa_Throw,Generator::instance->unreach,Generator::catches.back(),
          {excep,clazz.value().second->getClassInfos(builder).typeInfoPointVar,
          ConstantPointerNull::get(builder->getPtrTy())}),pos);
}

llvm::Value * NodeStmtCatch::generate(llvm::IRBuilder<> *builder) {
     Igel::internalErr("Cannot generate a Catch directly use NodeStmtTry");
}

llvm::Value * NodeStmtTry::generate(llvm::IRBuilder<> *builder) {
     llvm::FunctionCallee typeIdC = Generator::instance->m_module->getOrInsertFunction("llvm.eh.typeid.for",FunctionType::get(builder->getInt32Ty(),builder->getPtrTy(),false));
     llvm::FunctionCallee begin_Catch = Generator::instance->m_module->getOrInsertFunction("__cxa_begin_catch",FunctionType::get(builder->getPtrTy(),builder->getPtrTy(),false));
     llvm::FunctionCallee end_Catch = Generator::instance->m_module->getOrInsertFunction("__cxa_end_catch",{builder->getVoidTy()});

     Function* funcPers = Generator::instance->m_module->getFunction("__gxx_personality_v0");
     if(!funcPers) {
          Generator::instance->m_module->getOrInsertFunction("__gxx_personality_v0",{builder->getInt32Ty()});
          funcPers = Generator::instance->m_module->getFunction("__gxx_personality_v0");
     }
     Function* func = builder->GetInsertBlock()->getParent();
     func->setPersonalityFn(funcPers);

     BasicBlock* _catch = BasicBlock::Create(builder->getContext(), "catchM", func);
     BasicBlock* cont = nullptr;
     Generator::catches.push_back(_catch);
     Generator::catchCont.push_back(cont);
     generateS(scope,builder);
     if(!Generator::lastUnreachable)builder->CreateBr(cont);
     Generator::catchCont.pop_back();
     Generator::catches.pop_back();

     builder->SetInsertPoint(_catch);
     LandingPadInst* land = builder->CreateLandingPad(StructType::get(builder->getContext(),{builder->getPtrTy(),builder->getInt32Ty()}), 2);
     std::vector<BasicBlock*> catchesBB {};
     std::vector<BasicBlock*> catchSwitch {};

     Value* exP = builder->CreateExtractValue(land,0);
     Value* tid = builder->CreateExtractValue(land,1);

     for (const auto & i : catch_) {
          if(catch_.size() > 1) {
               BasicBlock* _cs = BasicBlock::Create(builder->getContext(), "catchswitch", func);
               catchSwitch.push_back(_cs);
          }

          BasicBlock* BB = BasicBlock::Create(builder->getContext(), "catch", func);
          builder->SetInsertPoint(BB);
          if(!catchesBB.empty())BB->moveAfter(catchesBB.back());

          Generator::lastUnreachable = false;
          Generator::instance->m_vars.emplace_back();

          auto clazz = Generator::findClass(i->typeName->mangle(),builder);
          Value* varRet = builder->CreateCall(begin_Catch,{exP});
          AllocaInst* alloc = builder->CreateAlloca(builder->getPtrTy());
          Value* varLoad = Igel::setDbg(builder,builder->CreateLoad(builder->getPtrTy(),varRet),pos);
          builder->CreateStore(varLoad,alloc);
          auto var = new ClassVar(alloc,false);
          var->type = builder->getPtrTy();
          var->strType = clazz.value().first;
          var->clazz = clazz.value().second;
          var->types = clazz.value().second->types;
          for(size_t i = 0;i < clazz.value().second->vars.size();i++) {
               NodeStmtLet* let = clazz.value().second->vars[i];
               std::string str = clazz.value().second->vars[i]->name;
               if(auto pirim = dynamic_cast<NodeStmtPirimitiv*>(let))var->signage.push_back(pirim->sid <= (char) TokenType::_long);
               else var->signage.push_back(false);
               var->vars[str] = i + 1;
          }
          for (uint i = 0; i < clazz.value().second->funcs.size();i++)var->funcs[clazz.value().second->funcs[i]->name] = i;
          Generator::instance->createVar("_Z" + std::to_string(i->varName.size()) + i->varName,var);

          for (const auto stmt : i->scope->stmts) {
               if(Generator::lastUnreachable)break;
               stmt->generate(builder);
          }
          if(!Generator::lastUnreachable) {
               if(!cont)cont = BasicBlock::Create(builder->getContext(), "cont", func);
               builder->CreateCall(end_Catch);
               builder->CreateBr(cont);
          }
          Generator::instance->m_vars.pop_back();

          catchesBB.push_back(BB);
          land->addClause(clazz->second->getClassInfos(builder).typeInfoPointVar);
     }

     builder->SetInsertPoint(_catch);
     if(catchesBB.size() == 1) {
          builder->CreateBr(catchesBB[0]);
     }else {
          for (int i = 0;i < catchSwitch.size() - 1;i++) {
               if(i == 0) {
                    catchSwitch[0]->moveAfter(_catch);
               }else {
                    catchSwitch[i]->moveAfter(catchSwitch[i] - 1);
               }
               builder->SetInsertPoint(catchSwitch[i]);

               std::optional<std::pair<StructType*,Class*>> clazz = Generator::findClass(catch_[i]->typeName->mangle(),builder);
               if(!clazz.has_value()) {
                    Igel::err("Could not find class " + catch_[i]->typeName->mangle(),pos);
               }
               Value* ctId = builder->CreateCall(typeIdC,clazz.value().second->getClassInfos(builder).typeInfo);
               Value* cmp = builder->CreateICmpEQ(ctId,tid);
               builder->CreateCondBr(cmp,catchesBB[i],catchSwitch[i + 1]);
          }
          catchSwitch.back()->moveAfter(catchSwitch[catchSwitch.size() - 2]);
          builder->SetInsertPoint(catchSwitch.back());
          builder->CreateBr(catchesBB[catchSwitch.size() - 1]);
     }

     if(!catchSwitch.empty())catchesBB.front()->moveAfter(catchSwitch.back());
     else catchesBB.front()->moveAfter(_catch);
     if(cont)cont->moveAfter(catchesBB.back());
     builder->SetInsertPoint(cont);

     return cont;
}

std::pair<llvm::Function*,FuncSig*> IgFunction::genFuncSig(llvm::IRBuilder<>* builder) {
     std::vector<Type*> types;
     if(member && !genSig) {
          paramType.insert(paramType.begin(),builder->getPtrTy());
          signage.insert(signage.begin(),false);
          paramName.insert(paramName.begin(),"this");
          paramTypeName.insert(paramTypeName.begin(),supper.value());
     }
     for(uint i = 0;i < paramType.size();i++) {
          types.push_back(paramType[i]);
     }
     FunctionType* type = FunctionType::get(_return,types,false);
     llvm::Function* llvmFunc = Function::Create(type,Function::ExternalLinkage,mangle(),*Generator::instance->m_module);
     for(size_t i = 0;i < llvmFunc->arg_size();i++)
          llvmFunc->getArg(i)->setName(paramName[llvmFunc->getArg(i)->getArgNo()]);
     genSig = true;
     return std::make_pair(llvmFunc,new FuncSig(mangle(),types,_return));
}

void IgFunction::genFunc(llvm::IRBuilder<>* builder) {
     if(gen)return;
     gen = true;
     Generator::lastUnreachable = false;
     Function* llvmFunc = Generator::instance->m_module->getFunction(mangle());
     BasicBlock* entry = BasicBlock::Create(*Generator::instance->m_contxt,"entry",llvmFunc);
     builder->SetInsertPoint(entry);

     if(Generator::instance->debug) {
          Debug dbg = Generator::instance->getDebug();
          std::vector<Metadata*> dbgTypes {};
          for (uint i = 0;i < paramType.size();i++) {
               dbgTypes.push_back(dbg.builder->createBasicType(paramName[i],Generator::instance->m_module->getDataLayout().getTypeSizeInBits(paramType[i]),
                    Generator::getEncodingOfType(paramType[i])));
          }
          DISubroutineType* subType = dbg.builder->createSubroutineType(dbg.builder->getOrCreateTypeArray(dbgTypes));
          DISubprogram* sub = dbg.builder->createFunction(dbg.file,name,mangle(),dbg.file,pos.line,subType,0,DINode::FlagPrototyped,DISubprogram::SPFlagDefinition);
          llvmFunc->setSubprogram(sub);
          Generator::instance->dbgScopes.push_back(sub);
     }

     Generator::instance->m_vars.emplace_back();
     if(supper.has_value()) {
          if(supper.value()->extending.has_value()) {
               llvmFunc->getArg(0)->setName("super");
               Generator::instance->createVar(llvmFunc->getArg(0),false,supper.value()->extending.value()->mangle());
          }
          llvmFunc->getArg(0)->setName("this");
          Generator::instance->createVar(llvmFunc->getArg(0),false,supper.value()->mangle());
     }
     for(size_t i = supper.has_value();i < llvmFunc->arg_size();i++){
          llvmFunc->getArg(i)->setName(paramName[i]);
          Generator::instance->createVar(llvmFunc->getArg(i),signage[i],llvmFunc->getArg(i)->getType()->isPointerTy()?(paramTypeName[i]?paramTypeName[i]->mangle():"_Z4this"):"");
     }

     scope->generate(builder);
     if(Generator::instance->debug)Generator::instance->dbgScopes.pop_back();
     Generator::instance->m_vars.pop_back();
     if(_return->isVoidTy())builder->CreateRetVoid();

     if(!Generator::lastUnreachable && !_return->isVoidTy()) {
          Igel::generalErr("No return statement in non void function " + mangle());
     }
     if(Generator::instance->unreach) {
          builder->SetInsertPoint(Generator::instance->unreach);
          builder->CreateUnreachable();
          Generator::instance->unreach = nullptr;
     }
     if(llvm::verifyFunction(*llvmFunc,&outs())) {
          //llvmFunc->print(outs());
          Generator::instance->m_module->print(outs(),nullptr);
          exit(EXIT_FAILURE);
     }
}

std::string IgFunction::mangle() {
     if(externC)return name;
     return Igel::Mangler::mangle(this,paramType,paramTypeName,signage,member,constructor,destructor);
}

llvm::Function* IgFunction::getLLVMFunc() {
     if(llvmFunc)return llvmFunc;
     if(!type)getLLVMFuncType();
     Generator::instance->m_module->getOrInsertFunction(mangle(),type);//Function::Create(type,Function::ExternalLinkage,func->mangle(),m_module.get());
     llvmFunc = Generator::instance->m_module->getFunction(mangle());
     return llvmFunc;
}

llvm::FunctionType* IgFunction::getLLVMFuncType() {
     if(type)return type;
     type = FunctionType::get(_return,paramType,false);
     return type;
}

void IgFunction::reset() {
     type = nullptr;
     llvmFunc = nullptr;
}

int Igel::getSize(TokenType type) {
     if(type > TokenType::_ulong)return -1;
     const int i = static_cast<int>(type) % 4;
     return  (i + 1) * 8 + 8 * std::max(i - 1,0) + 16 * std::max(i - 2,0);
}

void Struct::generateSig(llvm::IRBuilder<>* builder) {
     if(_extern)return;
     for(uint i = 0; i < this->types.size();i++) {
          if(types[i] == nullptr) {
               Igel::internalErr("Type is not set in " + mangle());
          }
          auto tIL = (ulong) types[i];
          switch (const ulong tI = tIL) {
               case 1:
               case 2:
                    types[i] = PointerType::get(builder->getContext(),0);
                    break;
               default:
                    break;
               case 0:
                    Igel::internalErr("Unknown type id: " + tI);
          }
     }
     strType = StructType::create(builder->getContext(),mangle());
     Generator::instance->m_file->structs[mangle()] = std::make_pair(strType,this);
}

void Struct::generatePart(llvm::IRBuilder<> *builder) {
     if(_extern)return;
     for (auto var : vars) {
          //TODO maybe mangle
          if(!var->_static)varIds.push_back(var->name);
     }
     for(size_t i = 0;i < vars.size();i++) {
          NodeStmtLet* let = vars[i];
          std::string str = vars[i]->name;
          if(auto pirim = dynamic_cast<NodeStmtPirimitiv*>(let))signage.push_back(pirim->sid <= (char) TokenType::_long);
          else signage.push_back(false);
          varIdMs[str] = i;
          finals[vars[i]->name] = vars[i]->final;
     }
     strType->setBody(types,false);
     for (auto static_var : staticVars) {
          static_var->generate(builder);
     }
}

void Struct::generate(llvm::IRBuilder<>* builder) {
     if(_extern)return;
     if(Generator::instance->debug) {
          Debug dbg = Generator::instance->getDebug();
          std::vector<Metadata*> dbgTypes {};
          //TODO add real offset with interfaces
          dbgType = dbg.builder->createStructType(nullptr,mangle(),dbg.file,pos.line,Generator::instance->m_module->getDataLayout().getTypeSizeInBits(strType),
          0,DINode::FlagNonTrivial | DINode::FlagTypePassByValue,nullptr,nullptr,
               0,nullptr,Igel::Mangler::mangle(this));
          uint offset = 0;

          for (uint i = 0;i < strType->getNumElements();i++) {
               auto subType = dbg.builder->createBasicType(varIds[i],Generator::instance->m_module->getDataLayout().getTypeSizeInBits(strType->getElementType(i)),
                    Generator::getEncodingOfType(types[i]));
               dbgTypes.push_back(dbg.builder->createMemberType(dbgType,varIds[i],dbg.file,0,Generator::instance->m_module->getDataLayout().getTypeSizeInBits(strType->getElementType(i)),
                    0,offset,DINode::FlagPublic,subType));
               offset += Generator::instance->m_module->getDataLayout().getTypeSizeInBits(strType->getElementType(i));
          }
          dbgType->replaceElements(dbg.builder->getOrCreateArray(dbgTypes));
     }
}

std::string Struct::mangle() {
     return Igel::Mangler::mangleTypeName(this);
}

llvm::StructType * Struct::getStrType(llvm::IRBuilder<> *builder) {
     if(strType)return strType;
     strType = StructType::create(builder->getContext(),mangle());
     Generator::instance->m_file->structs[mangle()] = std::make_pair(strType,this);
     strType->setBody(types,false);
     return strType;
}

void NamesSpace::generateSig(llvm::IRBuilder<>* builder) {
     if(_extern)return;
     for(auto func : funcs)func->genFuncSig(builder);
     for(auto cont : contained) {
          if(auto type = dynamic_cast<IgType*>(cont))type->generateSig(builder);
     }
}

void NamesSpace::generate(llvm::IRBuilder<>* builder) {
     if(_extern)return;
     for(auto func : funcs)func->genFunc(builder);
     for(auto cont : contained) {
          if(auto type = dynamic_cast<IgType*>(cont))type->generate(builder);
     }
}

std::string NamesSpace::mangle() {
     return Igel::Mangler::mangleTypeName(this);
}

std::string Interface::mangle() {
     return Igel::Mangler::mangleTypeName(this);
}

void Interface::generateSig(llvm::IRBuilder<> *builder) {
     if(_extern)return;
     Generator::instance->m_file->interfaces[mangle()] = this;
}

void Interface::generate(llvm::IRBuilder<> *builder) {
     if(_extern)return;
     if(gen)return;
     gen = true;
     Generator::instance->initInfo();
     classInfos.typeNameVar = builder->CreateGlobalString(Igel::Mangler::mangle(this),"_ZTS" + Igel::Mangler::mangle(this),0,Generator::instance->m_module.get());


     std::vector<Type*> typeInfoTypes {builder->getPtrTy(),builder->getPtrTy()};
     auto typeInfos = getTypeInfoRec(builder);
     typeInfoTypes.reserve(typeInfos.size());
     for (auto typeInfo : typeInfos) {
          typeInfoTypes.push_back(builder->getPtrTy());
     }
     typeInfos.insert(typeInfos.begin(),classInfos.typeNameVar);
     typeInfos.insert(typeInfos.begin(), (Constant*) (builder->CreateConstGEP1_64(builder->getPtrTy(), /*vtable*/ Generator::instance->cxx_class_type_info, 2)));

     StructType* tIT = StructType::get(builder->getContext(),typeInfoTypes);
     classInfos.typeInfo = new GlobalVariable(*Generator::instance->m_module,tIT,true,GlobalValue::ExternalLinkage,ConstantStruct::get(tIT,
          typeInfos),"_ZTI" + Igel::Mangler::mangle(this));
     classInfos.init = true;
}

ClassInfos Interface::getClassInfos(llvm::IRBuilder<> *builder) {
     if(!gen)generate(builder);
     if(classInfos.init)return classInfos;

     classInfos.typeInfo = new GlobalVariable(*Generator::instance->m_module,builder->getPtrTy(),false,GlobalValue::ExternalLinkage,nullptr,"_ZTI" + Igel::Mangler::mangle(this),
          nullptr,GlobalValue::NotThreadLocal,0,true);

     classInfos.typeNameVar = builder->CreateGlobalString(Igel::Mangler::mangle(this),"_ZTS" + Igel::Mangler::mangle(this),0,Generator::instance->m_module.get());

     classInfos.typeNamePointVar = builder->CreateGlobalString("P" + Igel::Mangler::mangle(this),"_ZTSP" + Igel::Mangler::mangle(this),0,Generator::instance->m_module.get());

     classInfos.init = true;
     return classInfos;
}

std::string Enum::mangle() {
     return Igel::Mangler::mangleTypeName(this);
}

void Enum::generateSig(llvm::IRBuilder<> *builder) {
     if(_extern)return;
     if(values.size() > std::pow(2,32)) {
          Igel::generalErr("Enum can not hava more then " + std::to_string(std::pow(2,32)) + " Entries \n    in: " + mangle());
     }
}

void Enum::generatePart(llvm::IRBuilder<> *builder) {
}

void Enum::generate(llvm::IRBuilder<> *builder) {
}

void Class::generateSig(llvm::IRBuilder<>* builder) {
     if(_extern)return;
     if(wasGen)return;
     wasGen = true;
     if(extending.has_value())extending.value()->generateSig(builder);
     for(uint i = 0; i < this->types.size();i++) {
          if(types[i] == nullptr) {
               Igel::generalErr("Type is not set in " + mangle());
          }
          auto tIL = (ulong) types[i];
          switch (const ulong tI = tIL) {
               case 1:
               case 2:
                    types[i] = PointerType::get(builder->getContext(),0);

               break;
               default:
                    break;
               case 0:
                    Igel::generalErr("Unkown type id: " + tI);

          }
     }

     for (auto constructor : constructors) {
          std::vector<Type*> types {builder->getPtrTy()};
          std::vector<BeContained*> names {new Name("_Z0")};
          std::vector<bool> sing = {true};

          if(!constructor->paramType.empty()) {
               types.insert(types.end(),constructor->paramType.begin(),constructor->paramType.end());
               names.insert(names.end(),constructor->paramTypeName.begin(),constructor->paramTypeName.end());
               sing.insert(sing.end(),constructor->signage.begin(),constructor->signage.end());
          }else {
               defaulfConstructor = constructor;
          }

          for (auto ig_function : constructors) {
               if(constructor == ig_function)continue;
               if(constructor->mangle() == ig_function->mangle()) {
                    Igel::generalErr("Same constructor definition: " + constructor->mangle());
               }
          }

          FunctionType* ty = FunctionType::get(builder->getVoidTy(),{types},false);
          Function* func = Function::Create(ty,GlobalValue::ExternalLinkage,Igel::Mangler::mangle(this,types,names,sing,true,true),*Generator::instance->m_module);
          constructor->_return = builder->getVoidTy();
          constructor->retTypeName = nullptr;
          func->getArg(0)->setName("this");
          for(size_t i = 1;i < func->arg_size();i++) {
               if(names[i])func->getArg(i)->setName(names[i]->mangle());
               else {
                    std::string str;
                    str.push_back(Igel::Mangler::typeToChar(types[i],sing[i]));
                    func->getArg(i)->setName(str);
               }
          }
          constructor->llvmFunc = func;
     }

     strType = StructType::create(builder->getContext(),mangle());

     Generator::instance->m_file->classes[mangle()] = {strType,this};
}

void Class::generatePart(llvm::IRBuilder<> *builder) {
     if(_extern)return;
     if(partGen)return;
     partGen = true;
     if(extending.has_value())extending.value()->generatePart(builder);

     auto fullFuncs = getFuncsRec();
     for(int i = 0;i < implementing.size() /*+ templateConstraints.size()*/;i++)types.insert(types.begin(),builder->getPtrTy());
     if(extending.has_value())types.insert(types.begin(),extending.value()->strType);
     else types.insert(types.begin(),builder->getPtrTy());
     strType->setBody(types,false);

     vTablePtrs = implementing.size() + 1;
     uint j = 0;
     for (size_t i = 0;i < vars.size();i++) {
          if(!vars[i]->_static) {
               std::string str = vars[i]->name;
               varNames.push_back(str);
               varIdMs[str] = j + implementing.size() + 1 /*+ templateConstraints.size()*/;
               varAccesses[str] = vars[i]->acc;
               finals[str] = vars[i]->final;
               if (vars[i]->isTemplate) {
                    templateIdMs[vars[i]->name] = vars[i]->templateId;
               }
               j++;
          }
     }
}

void Class::generate(llvm::IRBuilder<>* builder) {
     if(_extern)return;
     if(fullGen)return;
     fullGen = true;
     if(!partGen)generatePart(builder);
     if(extending.has_value())extending.value()->generate(builder);

     if(extending.has_value() && extending.value()->final) {
          Igel::generalErr("Cannot extend final class\n in: " + mangle());
     }

     auto fullFuncs = getFuncsRec();

     for (int i = 0; i < funcs.size();i++) {
          funcs[i]->member = true;
          funcs[i]->supper = this;

          if(!funcs[i]->abstract) {
               funcs[i]->genFuncSig(builder);
          }else {
               std::vector<Type*> types;
               for(uint j = 0;j < funcs[i]->paramType.size();j++) {
                    types.push_back(funcs[i]->paramType[j]);
               }
          }
     }

     Type* ptr = PointerType::get(*Generator::instance->m_contxt,0);

     Generator::instance->initInfo();

     //TODO make extending and make throwables extend Exception
     classInfos.typeNameVar = builder->CreateGlobalString(Igel::Mangler::mangle(this),"_ZTS" + Igel::Mangler::mangle(this),0,Generator::instance->m_module.get());
     classInfos.typeNamePointVar = builder->CreateGlobalString("P" + Igel::Mangler::mangle(this),"_ZTSP" + Igel::Mangler::mangle(this),0,Generator::instance->m_module.get());
     std::vector<Type*> typeInfoTypes {builder->getPtrTy(),builder->getPtrTy()};
     auto typeInfos = getTypeInfoRec(builder);
     typeInfoTypes.reserve(typeInfos.size());
     for (auto typeInfo : typeInfos) {
          typeInfoTypes.push_back(builder->getPtrTy());
     }
     typeInfos.insert(typeInfos.begin(),classInfos.typeNameVar);
     typeInfos.insert(typeInfos.begin(), (Constant*) (builder->CreateConstGEP1_64(ptr, /*vtable*/ Generator::instance->cxx_class_type_info, 2)));

     StructType* tIT = StructType::get(builder->getContext(),typeInfoTypes);
     classInfos.typeInfo = new GlobalVariable(*Generator::instance->m_module,tIT,true,GlobalValue::ExternalLinkage,ConstantStruct::get(tIT,
          typeInfos),"_ZTI" + Igel::Mangler::mangle(this));
     StructType* tIPT = StructType::get(*Generator::instance->m_contxt,{builder->getPtrTy(),builder->getPtrTy(),builder->getInt32Ty(),builder->getPtrTy()});
     classInfos.typeInfoPointVar = new GlobalVariable(*Generator::instance->m_module,tIPT,true,GlobalVariable::ExternalLinkage,ConstantStruct::get(tIPT,
          {(Constant*) builder->CreateConstGEP1_64(ptr,Generator::instance->cxx_pointer_type_info,2),classInfos.typeNamePointVar,
               ConstantInt::get(builder->getInt32Ty(),0),classInfos.typeInfo}),"_ZTIP" + Igel::Mangler::mangle(this));
     std::vector<Constant*> vtablefuncs {ConstantPointerNull::get(builder->getPtrTy()),classInfos.typeInfo};

     //insert template types
     Generator::templateStack.emplace_back();
     if(templateNames.size() != templateConstraints.size())Igel::err("Template name size has to match constraint size",pos);
     for(int i = 0;i < templateNames.size();i++){
          Generator::templateStack.back()[templateNames[i]] = templateConstraints[i];
     }

     //TODO insert destructors into vtable


     std::vector<IgFunction*> overides {};
     for (auto full_func : fullFuncs[0]) {
          if(full_func->_override) {
               overides.push_back(full_func);
               //TODO this may remove every element after the one to be removed
               fullFuncs[0].erase(std::ranges::remove(fullFuncs[0],full_func).begin(),fullFuncs[0].end());
          }
     }

     for (auto & fullFunc : fullFuncs) {
          for (auto & j : fullFunc) {
               IgFunction* func = j;
               for (auto overide : overides) {
                    if(func->name != overide->name)continue;
                    j = overide;
               }
          }
     }

     std::vector<Constant*> vtables;
          vtables.reserve(fullFuncs.size());
          ulong supperOffset = 0;
          ulong extImplSize = 0;
          if(extending.has_value()) {
               supperOffset = Generator::instance->m_module->getDataLayout().getTypeSizeInBits(extending.value()->strType) / 8 - 8;
               extImplSize = extending.value()->implementing.size();
          }
          for (int i = 0;i < fullFuncs.size();i++) {
               std::vector<Constant*> vtablefuncsimp {(Constant*) builder->CreateIntToPtr(ConstantInt::get(builder->getInt64Ty(),i * -8 - supperOffset * (i > 0)),builder->getPtrTy()),
                    classInfos.typeInfo};
               vtablefuncsimp.reserve(fullFuncs[i].size());
               for(int j = 0;j < fullFuncs[i].size();j++) {
                    if(!abstract) {
                         if(fullFuncs[i][j]->abstract) {
                              Igel::generalErr("Abstract function has either to be declared or the class has to be declared abstract\n     in: " + mangle());
                         }
                         vtablefuncsimp.push_back(fullFuncs[i][j]->getLLVMFunc());
                    }else {
                         vtablefuncsimp.push_back(Generator::instance->cxx_pure_virtual);
                    }
                    funcIdMs[fullFuncs[i][j]->mangle()] = std::make_pair(i + supperOffset / 8 * (i > 0),j);
                    funcMap[i + supperOffset / 8 * (i > extImplSize)][j] = fullFuncs[i][j];
               }
               vtables.push_back(ConstantArray::get(ArrayType::get(ptr,fullFuncs[i].size() + 2 /*offset and type info*/ + 0 /*Destructors*/),{vtablefuncsimp}));
          }

     for (int i = 0; i < funcs.size();i++) {
          if(!funcs[i]->abstract) {
               funcs[i]->genFunc(builder);
          }
     }

     std::vector<Type*> vtableTypes {};
     vtableTypes.reserve(vtables.size());
     for (auto vt : vtables) vtableTypes.push_back(vt->getType());

     Constant* vtableStr = ConstantStruct::get(StructType::get(builder->getContext(),vtableTypes),vtables);

     classInfos.vtable = new GlobalVariable(*Generator::instance->m_module,vtableStr->getType()/*vtableInit->getType()*/,true,GlobalValue::ExternalLinkage,vtableStr/*vtableInit*/,
          "_ZTV" + Igel::Mangler::mangle(this));

     //TODO insert destructors into vtable
     if(Generator::instance->debug) {
          Debug dbg = Generator::instance->getDebug();
          std::vector<Metadata*> dbgTypes {};
          //TODO add real offset with interfaces
          dbgType = dbg.builder->createClassType(nullptr,mangle(),dbg.file,pos.line,Generator::instance->m_module->getDataLayout().getTypeSizeInBits(strType),
          0,0,DINode::FlagNonTrivial | DINode::FlagTypePassByValue,
               extending.has_value()?extending.value()->dbgType:nullptr,nullptr,
               0,nullptr,nullptr,Igel::Mangler::mangle(this));
          uint offset = 0;
          if(extending.has_value()) {
               dbgTypes.push_back(dbg.builder->createInheritance(dbgType,extending.value()->dbgType,getSuperOffset(extending.value()),0,DINode::FlagPublic));
               offset = Generator::instance->m_module->getDataLayout().getTypeSizeInBits(extending.value()->strType);
          }else offset = 64;
          offset += implementing.size() * 64;

          for (uint i = implementing.size() + 1 + templateConstraints.size();i < strType->getNumElements();i++) {
               auto subType = dbg.builder->createBasicType(varNames[i - implementing.size() - 1 - templateConstraints.size()],Generator::instance->m_module->getDataLayout().getTypeSizeInBits(strType->getElementType(i)),
                    Generator::getEncodingOfType(types[i]));
               dbgTypes.push_back(dbg.builder->createMemberType(dbgType,varNames[i - implementing.size() - 1 - templateConstraints.size()],dbg.file,0,Generator::instance->m_module->getDataLayout().getTypeSizeInBits(strType->getElementType(i)),
                    0,offset,DINode::FlagPublic,subType));
               offset += Generator::instance->m_module->getDataLayout().getTypeSizeInBits(strType->getElementType(i));
          }
          dbgType->replaceElements(dbg.builder->getOrCreateArray(dbgTypes));
          dbgpointerType = dbg.builder->createPointerType(dbgType,Generator::instance->m_module->getDataLayout().getTypeSizeInBits(strType));
     }
     for (auto constructor : constructors) {
          BasicBlock* entry = BasicBlock::Create(*Generator::instance->m_contxt,"entry",constructor->getLLVMFunc());
          constructor->getLLVMFunc()->getArg(0)->setName("this");
          builder->SetInsertPoint(entry);

          if(Generator::instance->debug) {
               Debug dbg = Generator::instance->getDebug();
               std::vector<Metadata*> dbgTypes {};
               for (uint i = 0;i < constructor->getLLVMFunc()->arg_size();i++) {
                    dbgTypes.push_back(dbg.builder->createBasicType(constructor->getLLVMFunc()->getArg(i)->getName(),
                         Generator::instance->m_module->getDataLayout().getTypeSizeInBits(constructor->getLLVMFunc()->getArg(i)->getType()),dwarf::DW_TAG_member));
               }
               DISubroutineType* subType = dbg.builder->createSubroutineType(dbg.builder->getOrCreateTypeArray(dbgTypes));
               DISubprogram* sub = dbg.builder->createFunction(dbg.file,name,mangle(),dbg.file,constructor->pos.line,subType,0,
                    DINode::FlagPrototyped,DISubprogram::SPFlagDefinition);
               constructor->llvmFunc->setSubprogram(sub);
               Generator::instance->dbgScopes.push_back(sub);
          }

          AllocaInst* alloc = builder->CreateAlloca(ptr);
          builder->CreateStore(constructor->getLLVMFunc()->getArg(0),alloc);
          Instruction* load = Igel::setDbg(builder,builder->CreateLoad(alloc->getAllocatedType(),alloc),constructor->pos);

          Generator::instance->m_vars.emplace_back();
          Generator::instance->createVar(constructor->getLLVMFunc()->getArg(0),false,this->mangle());

          bool supConstCall = false;
          if(!constructor->scope->stmts.empty() && dynamic_cast<NodeStmtSuperConstructorCall*>(constructor->scope->stmts[0])) {
               constructor->scope->startIndex = 1;
               constructor->scope->stmts[0]->generate(builder);
               supConstCall = true;
          }

          if(extending.has_value() && extending.value()->defaulfConstructor.has_value() && !supConstCall)Igel::setDbg(builder,builder->CreateCall(extending.value()->defaulfConstructor.value()->getLLVMFunc(),
               constructor->getLLVMFunc()->getArg(0)),pos);

          builder->SetInsertPoint(entry);

          Constant* vtG = ConstantExpr::getGetElementPtr(classInfos.vtable->getValueType(),classInfos.vtable,
               (ArrayRef<Constant*>) {builder->getInt32(0),builder->getInt32(0),builder->getInt32(2)},true,1);
          builder->CreateStore(vtG,load);
          for(int i = 1;i < fullFuncs.size();i++) {
               Value* ptr2 = builder->CreateInBoundsGEP(builder->getInt8Ty(),load,ConstantInt::get(builder->getInt64Ty(), i * 8 + supperOffset));
               Constant* vtGI = ConstantExpr::getGetElementPtr(classInfos.vtable->getValueType(),classInfos.vtable,
                    (ArrayRef<Constant*>) {builder->getInt32(0),builder->getInt32(i),builder->getInt32(2)},true,1);
               builder->CreateStore(vtGI,ptr2);
          }

          for(size_t i = 1;i < constructor->getLLVMFunc()->arg_size();i++){
               constructor->getLLVMFunc()->getArg(i)->setName(constructor->paramName[i - 1]);
               Generator::instance->createVar(constructor->getLLVMFunc()->getArg(i),constructor->signage[i - 1],
                    constructor->getLLVMFunc()->getArg(i)->getType()->isPointerTy()?constructor->paramTypeName[i - 1]->mangle():"",false);
          }

          uint j = 0;
          for (size_t i = 0;i < vars.size();i++) {
               if(!vars[i]->_static) {
                    Value* val = builder->CreateStructGEP(strType,load,j + implementing.size() + 1);
                    Value* gen = vars[i]->generatePointer(builder);
                    builder->CreateStore(gen,val);
                    j++;
               }else vars[i]->generate(builder);
          }

          constructor->scope->generate(builder);
          Generator::instance->m_vars.pop_back();

          builder->CreateRetVoid();
          builder->SetInsertPoint((BasicBlock*) nullptr);
          if(Generator::instance->debug)Generator::instance->dbgScopes.pop_back();
     }

     if(constructors.empty()) {
          FunctionType* ty = FunctionType::get(builder->getVoidTy(),{builder->getPtrTy()},false);
          Function* func = Function::Create(ty,GlobalValue::ExternalLinkage,Igel::Mangler::mangle(this,{},{},{},true,true),*Generator::instance->m_module);
          BasicBlock* entry = BasicBlock::Create(*Generator::instance->m_contxt,"entry",func);
          builder->SetInsertPoint(entry);

         if(Generator::instance->debug) {
               Debug dbg = Generator::instance->getDebug();
               std::vector<Metadata*> dbgTypes {};

               dbgTypes.push_back(dbg.builder->createBasicType("_Z4this",
                    Generator::instance->m_module->getDataLayout().getTypeSizeInBits(builder->getPtrTy()),dwarf::DW_TAG_member));

               DISubroutineType* subType = dbg.builder->createSubroutineType(dbg.builder->getOrCreateTypeArray(dbgTypes));
               DISubprogram* sub = dbg.builder->createFunction(dbg.file,name,mangle(),dbg.file,pos.line,subType,0,
                    DINode::FlagPrototyped,DISubprogram::SPFlagDefinition);
               func->setSubprogram(sub);
               Generator::instance->dbgScopes.push_back(sub);
          }

          AllocaInst* alloc = builder->CreateAlloca(ptr);
          builder->CreateStore(func->getArg(0),alloc);
          Instruction* load = builder->CreateLoad(alloc->getAllocatedType(),alloc);

          if(extending.has_value() && extending.value()->defaulfConstructor.has_value()) {
               Igel::setDbg(builder,builder->CreateCall(extending.value()->defaulfConstructor.value()->getLLVMFunc(),func->getArg(0)),pos);
          }
          if(extending.has_value() && !extending.value()->defaulfConstructor) {
               Igel::generalErr("No default constructor for " + extending.value()->mangle() + " manual call to super() needed in " + mangle());
          }

          builder->SetInsertPoint(entry);


          Constant* vtG = ConstantExpr::getGetElementPtr(classInfos.vtable->getValueType(),classInfos.vtable,(ArrayRef<Constant*>) {builder->getInt32(0),builder->getInt32(0),builder->getInt32(2)},
          true,1);
          builder->CreateStore(vtG,load);

          for(int i = 1;i < fullFuncs.size();i++) {
               if(fullFuncs[i].empty())continue;
               Value* ptr2 = builder->CreateInBoundsGEP(builder->getInt8Ty(),load,ConstantInt::get(builder->getInt64Ty(), i * 8 + supperOffset));
               Constant* vtGI = ConstantExpr::getGetElementPtr(classInfos.vtable->getValueType(),classInfos.vtable,(ArrayRef<Constant*>) {builder->getInt32(0),builder->getInt32(i),builder->getInt32(2)},
                    true,1);
               builder->CreateStore(vtGI,ptr2);
          }

          uint j = 0;
          for (size_t i = 0;i < vars.size();i++) {
               if(!vars[i]->_static) {
                    Generator::clearTypeNameRet();
                    Value* val = builder->CreateStructGEP(strType,load,j + implementing.size() + 1);
                    Value* gen = vars[i]->generatePointer(builder);
                    builder->CreateStore(gen,val);
                    if(Generator::instance->debug) {
                         Debug dbg = Generator::instance->getDebug();
                         DIScope* scope = Generator::instance->dbgScopes.empty()?dbg.file:Generator::instance->dbgScopes.back();
                         DIType* type = nullptr;
                         if(Generator::typeNameRet && dynamic_cast<IgType*>(Generator::typeNameRet->type) &&  dynamic_cast<IgType*>(Generator::typeNameRet->type)->dbgType) {
                              type = dynamic_cast<IgType*>(Generator::typeNameRet->type)->dbgType;
                         }
                         else type = dbg.builder->createBasicType(name,Generator::instance->m_module->getDataLayout().getTypeSizeInBits(val->getType()),Generator::getEncodingOfType(val->getType()));
                         auto dbgVar = dbg.builder->createAutoVariable(scope,name,dbg.file,pos.line,type);
                         dbg.builder->insertDeclare(val,dbgVar,dbg.builder->createExpression(),DILocation::get(builder->getContext(),pos.line,pos._char,scope),builder->GetInsertBlock());
                    }
                    j++;
               }else vars[i]->generate(builder);
          }
          builder->CreateRetVoid();
          auto igFunc = new IgFunction();
          igFunc->name = mangle();
          igFunc->constructor = true;
          igFunc->member = true;
          igFunc->paramName = {"this"};
          igFunc->paramTypeName = {this};
          igFunc->paramType = {builder->getPtrTy()};
          igFunc->signage = {false};
          igFunc->supper = this;
          igFunc->_return = builder->getVoidTy();
          this->defaulfConstructor = igFunc;
          if(Generator::instance->debug)Generator::instance->dbgScopes.pop_back();
     }
     hasConstructor = true;
     classInfos.init = true;

     Generator::templateStack.pop_back();
}

std::string Class::mangle() {
     return Igel::Mangler::mangleTypeName(this);
}

ClassInfos Class::getClassInfos(llvm::IRBuilder<> *builder) {
     if(!fullGen)generate(builder);
     if(classInfos.init)return classInfos;

     classInfos.vtable = new GlobalVariable(*Generator::instance->m_module,builder->getPtrTy(),false,GlobalValue::ExternalLinkage,nullptr,"_ZTV" + Igel::Mangler::mangle(this),
          nullptr,GlobalValue::NotThreadLocal,0,true);
     classInfos.typeInfo = new GlobalVariable(*Generator::instance->m_module,builder->getPtrTy(),false,GlobalValue::ExternalLinkage,nullptr,"_ZTI" + Igel::Mangler::mangle(this),
          nullptr,GlobalValue::NotThreadLocal,0,true);
     classInfos.typeInfoPointVar = new GlobalVariable(*Generator::instance->m_module,builder->getPtrTy(),false,GlobalValue::ExternalLinkage,nullptr,"_ZTIP" + Igel::Mangler::mangle(this),
          nullptr,GlobalValue::NotThreadLocal,0,true);
     classInfos.typeNameVar = builder->CreateGlobalString(Igel::Mangler::mangle(this),"_ZTS" + Igel::Mangler::mangle(this),0,Generator::instance->m_module.get());
     classInfos.typeNamePointVar = builder->CreateGlobalString("P" + Igel::Mangler::mangle(this),"_ZTSP" + Igel::Mangler::mangle(this),0,Generator::instance->m_module.get());

     classInfos.init = true;
     return classInfos;
}

uint Class::getExtendingOffset() {
     return Generator::instance->dataLayout.getTypeSizeInBits(extending.value()->strType);
}

uint Class::getSuperOffset(ContainableType *type) {
     if(!isSubTypeOf(type))return -1;
     if(extending.has_value() && extending.value()->isSubTypeOf(type)) return extending.value()->getSuperOffset(type);
     uint offset = extending.has_value()?Generator::instance->dataLayout.getTypeSizeInBits(extending.value()->strType):0;
     for (auto interface : implementing) {
          if(interface == type || interface->isSubTypeOf(type))return offset;
          offset += interface->getExtendingOffset();
     }
     return 0;
}

bool Igel::SecurityManager::canAccess(ContainableType *type, IgFunction *func) {
     if(func->acc.isPublic())return true;
     if(!func->supper || !type)return false;
     if(func->acc.isPrivate())return func->supper.value() == type;
     if(dynamic_cast<PolymorphicType*>(type) && func->acc.isProtected())return dynamic_cast<PolymorphicType*>(type)->isSubTypeOf(func->supper.value());
     return false;
}

bool Igel::SecurityManager::canAccess(ContainableType* accesed, ContainableType *accessor, Access access) {
     if(access.isPublic())return true;
     if(!accesed || ! accessor)return false;
     if(access.isPrivate())return accesed == accessor;
     if(dynamic_cast<PolymorphicType*>(accessor) &&  access.isProtected())return dynamic_cast<PolymorphicType*>(accessor)->isSubTypeOf(accesed);
     return false;
}

std::pair<llvm::FunctionCallee,bool> getFunction(std::string name, std::vector<llvm::Type *> types) {
     std::optional<std::pair<llvm::FunctionCallee,bool>> callee = Generator::instance->m_file->findFunc(name,types);
     if(!callee.has_value()) {
          std::cerr << "Could not find function " << name << "\n";
          exit(EXIT_FAILURE);
     }
     return callee.value();
}

std::pair<llvm::FunctionCallee, bool> getFunction(BeContained* name, std::vector<llvm::Type *> types,std::vector<BeContained*> typeNames, std::vector<bool> signage) {
     return getFunction(Igel::Mangler::mangle(name,types,typeNames,signage),types);
}

llvm::Type* getType(TokenType type) {
     if(type == TokenType::id || type == TokenType::uninit)return PointerType::get(*Generator::instance->m_contxt,0);
     if(type <= TokenType::_ulong) {
          int i = static_cast<int>(type) % 4;
          int b = (i + 1) * 8 + 8 * std::max(i - 1,0) + 16 * std::max(i - 2,0);
          return llvm::IntegerType::get(*Generator::instance->m_contxt,b);
     }
     if(type == TokenType::_float)return llvm::IntegerType::getFloatTy(*Generator::instance->m_contxt);
     if(type == TokenType::_double)return llvm::IntegerType::getDoubleTy(*Generator::instance->m_contxt);
     if(type == TokenType::_bool)return llvm::IntegerType::getInt1Ty(*Generator::instance->m_contxt);
     if(type == TokenType::_void)return Type::getVoidTy(*Generator::instance->m_contxt);
     return nullptr;
}

llvm::Value* generateReassing(llvm::Value* load,llvm::Value* _new,TokenType op,llvm::IRBuilder<>* builder) {
     switch (op) {
          case TokenType::inc:
               return  builder->CreateAdd(load,ConstantInt::get(load->getType(),1));
          case TokenType::dec:
               return builder->CreateSub(load,ConstantInt::get(load->getType(),1));
          case TokenType::plus_eq:
               return builder->CreateAdd(load,_new);
          case TokenType::sub_eq:
               return builder->CreateSub(load,_new);
          case TokenType::mul_eq:
               return builder->CreateMul(load,_new);
          case TokenType::div_eq:
               return builder->CreateSDiv(load,_new);
          default:
               std::cerr << "Unexpected operator" << (uint) op;
               exit(EXIT_FAILURE);
     }
}
llvm::Value* NodeTermIntLit::generate(llvm::IRBuilder<>* builder) {
     llvm::Type* type = Generator::getType(static_cast<TokenType>(sid));
     if(sid >= 8 && sid <= 9)return llvm::ConstantFP::get(type,std::stod(int_lit.value.value()));
     int base = 10;
     std::string val = int_lit.value.value();
     if(int_lit.value.value().size() > 2) {
          if(int_lit.value.value()[0] == '0' && int_lit.value.value()[1] == 'x') {
               base = 16;
               val = val.substr(2);
          }else if(int_lit.value.value()[0] == '0' && int_lit.value.value()[1] == 'b') {
               base = 2;
               val = val.substr(2);
          }
     }
     return  llvm::ConstantInt::get(type,std::stol(val,nullptr,base),sid <= 3);
}

llvm::Value * NodeTermStringLit::generate(llvm::IRBuilder<> *builder) {
     Constant* strGl = builder->CreateGlobalStringPtr(str.value.value());
     if(Generator::instance->debug) {
          Debug dbg = Generator::instance->getDebug();
          auto tst = dbg.builder->createGlobalVariableExpression(Generator::instance->getScope(),strGl->getName(),strGl->getName(),
               dbg.file,pos.line,dbg.builder->createStringType(strGl->getName(),str.value.value().size()),true);
     }
     Generator::setTypeNameRet(new Name("c"));
     Generator::typeNameRet->type->mangleThis = false;
     return strGl;
}

llvm::Value* NodeTermFuncCall::generate(llvm::IRBuilder<>* builder) {
     Generator::classRet = nullptr;
     if(contained) {
          Generator::stump = true;
          contained.value()->generatePointer(builder);
          Generator::stump = false;
          if(Generator::classRet) {
               if(auto clz = dynamic_cast<NodeTermAcces*>(contained.value())) {
                    if(!dynamic_cast<Class*>(Generator::classRet))Igel::err("This opperation can only be performed on classes",pos);
                    auto clazz = dynamic_cast<Class*>(Generator::classRet);
                    if(auto id = dynamic_cast<NodeTermAcces*>(contained.value())) {
                         if(id->id->name == "super") {
                              auto func = clazz->getFunction(name->name);
                              if(!func) {
                                   Igel::err("Unknown function: " + name->mangle(),pos);
                              }

                              if(!Igel::SecurityManager::canAccess(superType,func.value())) {
                                   Igel::err("Cannot access function: " + name->mangle(),pos);
                              }
                              auto ptr = clz->generateClassPointer(builder);
                              std::vector<PolymorphicType*> templates = Generator::typeNameRet->templateTypes;
                              std::vector<Value*> vals {ptr};
                              Igel::check_Pointer(builder,ptr);
                              vals.reserve(exprs.size());
                              for (auto expr : exprs) vals.push_back(expr->generate(builder));
                              Generator::structRet = dynamic_cast<Struct*>(func.value()->retTypeName->type);
                              Generator::classRet = dynamic_cast<Interface*>(func.value()->retTypeName->type);
                              if (Generator::classRet && func.value()->templateId != -1) {
                                   Generator::classRet == templates[func.value()->templateId];
                                   Generator::setTypeNameRet(templates[func.value()->templateId]);
                              }
                              return Igel::setDbg(builder,builder->CreateCall(func.value()->getLLVMFunc(),vals),pos);
                         }
                    }
                    auto ptr = clz->generateClassPointer(builder);
                    std::vector<PolymorphicType*> templates = Generator::typeNameRet->templateTypes;
                    std::vector<Value*> vals{ptr};
                    Igel::check_Pointer(builder,ptr);
                    vals.reserve(exprs.size());

                    std::vector<Type*> types;
                    std::vector<BeContained*> typeNames;
                    std::vector<bool> signage;
                    for (auto expr : exprs) {
                         Generator::clearTypeNameRet();
                         auto exprVal = expr->generate(builder);
                         vals.push_back(exprVal);
                         types.push_back(exprVal->getType());
                         typeNames.push_back(Generator::typeNameRet->type);
                         signage.push_back(expr->_signed);
                    }
                    Generator::clearTypeNameRet();

                    bool found = false;
                    std::pair<uint,uint> fid = {};
                    if(clazz->funcIdMs.contains(name->name)) {
                         fid = clazz->funcIdMs[name->name];
                         found = true;
                    }
                    if(!found) {
                         if(!(name->contType && name->contType.value()))name->contType = clazz;
                         auto funcName = Igel::Mangler::mangle(name,types,typeNames,signage,false,false);
                         if(clazz->funcIdMs.contains(funcName)) {
                              fid = clazz->funcIdMs[funcName];
                              found = true;
                         }

                    }
                    if(!found){
                         auto funcName = Igel::Mangler::mangle(name,types,typeNames,signage,false,false);
                         if(clazz->directFuncs.contains(funcName)) {
                              auto func = clazz->directFuncs[funcName];
                              return Igel::setDbg(builder,builder->CreateCall(func->getLLVMFunc(),vals),pos);
                         }
                    }
                    if(!found) {
                         auto funcName = Igel::Mangler::mangle(name,types,typeNames,signage,false,false);
                         Igel::err("Unknown function: " + name->mangle(),pos);
                    }

                    auto ptrP = builder->CreateInBoundsGEP(builder->getPtrTy(),ptr,builder->getInt64(fid.first));
                    auto vtable = Igel::setDbg(builder,builder->CreateLoad(ptr->getType(),ptrP),pos);

                    auto func = clazz->funcMap[fid.first][fid.second];
                    if(!Igel::SecurityManager::canAccess(superType,func)) {
                         Igel::err("Cannot access function: " + name->mangle(),pos);
                    }
                    auto funcPtrGEP = builder->CreateInBoundsGEP(vtable->getType(),vtable,builder->getInt64(fid.second));
                    auto funcPtr = Igel::setDbg(builder,builder->CreateLoad(builder->getPtrTy(),funcPtrGEP),pos);

                    Generator::structRet = dynamic_cast<Struct*>(func->retTypeName->type);
                    Generator::classRet = dynamic_cast<Interface*>(func->retTypeName->type);
                    if (Generator::classRet && func->templateId != -1) {
                         Generator::classRet = templates[func->templateId];
                         Generator::setTypeNameRet(templates[func->templateId]);
                    }

                    return Igel::setDbg(builder,builder->CreateCall(func->getLLVMFuncType(),funcPtr,vals),pos);
               }
          }else contained.value()->generate(builder);
     }
     std::vector<llvm::Value*> vals;
     vals.reserve(exprs.size());
     typeNames.clear();
     typeNames.reserve(exprs.size());
     for (const auto expr : exprs) {
          llvm::Value* val = expr->generate(builder);
          if(val->getType()->isPointerTy() || val->getType()->isIntegerTy(0))typeNames.push_back(Generator::typeNameRet->type);
          vals.push_back(val);
     }

     if(params.empty()) {
          params.reserve(exprs.size());
          for (auto val : vals) {
               params.push_back(val->getType());
          }
     }
     if(signage.empty()) {
          signage.reserve(exprs.size());
          for (auto expr : exprs) signage.push_back(expr->_signed);
     }
     const std::pair<llvm::FunctionCallee,bool> callee = getFunction(name,params,typeNames,signage);
     _signed = callee.second;
     llvm::Value* val =  Igel::setDbg(builder,builder->CreateCall(callee.first,vals),pos);
     return val;
}

llvm::Value* NodeTermClassNew::generate(llvm::IRBuilder<>* builder) {
     if(auto clazz = Generator::findClass(typeName->mangle(),builder)){
          std::vector<Type*> types {builder->getPtrTy()};
          std::vector<BeContained*> names {new Name("")};
          std::vector<bool> sing = {true};

          std::vector<Value*> params {};
          params.reserve(exprs.size());
          if(paramType.empty() && !exprs.empty()) {
               for (auto expr : exprs) {
                    auto exprRet = expr->generate(builder);
                    params.push_back(exprRet);
                    Generator::clearTypeNameRet();
                    types.push_back(exprRet->getType());
                    names.push_back(Generator::typeNameRet->type);
                    sing.push_back(expr->_signed);
                    Generator::clearTypeNameRet();
               }
          }
          if(!paramType.empty()) {
               types.insert(types.end(),paramType.begin(),paramType.end());
               names.insert(names.end(),paramTypeName.begin(),paramTypeName.end());
               sing.insert(sing.end(),signage.begin(),signage.end());
          }

          FunctionType* type = FunctionType::get(PointerType::get(builder->getContext(),0),{Type::getInt64Ty(builder->getContext())},false);
          const llvm::FunctionCallee _new = Generator::instance->m_module->getOrInsertFunction("GC_malloc",type);
          Value* ptr = builder->CreateCall(_new,ConstantInt::get(Type::getInt64Ty(builder->getContext()),Generator::instance->m_module->getDataLayout().getTypeSizeInBits(clazz.value().first) / 8));
          if(clazz.value().second->hasConstructor) {
               ArrayRef<Type*> llvmTypes {types};
               auto calle = Generator::instance->m_module->getOrInsertFunction(Igel::Mangler::mangle(typeName,types,names,sing,true,true),builder->getVoidTy(),llvmTypes);

               params.insert(params.begin(),ptr);
               Igel::setDbg(builder,builder->CreateCall(calle,params),pos);
          }

          Generator::setTypeNameRet(typeName,templateArgs);
          return ptr;
     }
     Igel::err("Unknown type " + typeName->mangle(),pos);
}

llvm::Value* NodeTermArrNew::generate(llvm::IRBuilder<>* builder) {
     const llvm::FunctionCallee _sizNes = Generator::instance->m_module->getOrInsertFunction("malloc",PointerType::get(builder->getContext(),0),IntegerType::getInt64Ty(builder->getContext()));
     Value* sizes = builder->CreateCall(_sizNes,ConstantInt::get(IntegerType::getInt64Ty(builder->getContext()),8 * size.size()));
     Value* ptr = builder->CreateInBoundsGEP(IntegerType::getInt64Ty(builder->getContext()),sizes,ConstantInt::get(IntegerType::getInt64Ty(builder->getContext()),0));
     Value* sz = size.back()->generate(builder);
     if(!sz->getType()->isIntegerTy(64))sz = builder->CreateSExt(sz,builder->getInt64Ty());
     builder->CreateStore(sz,ptr);
     uint j = 1;
     for (uint i = size.size() - 2; i < 0xFFFFFFFF;i--) {
          sz = size[i]->generate(builder);
          if(!sz->getType()->isIntegerTy(64))sz = builder->CreateSExt(sz,builder->getInt64Ty());
          ptr = builder->CreateInBoundsGEP(IntegerType::getInt64Ty(builder->getContext()),sizes,ConstantInt::get(IntegerType::getInt64Ty(builder->getContext()),j));
          builder->CreateStore(sz,ptr);
          j++;
     }
     const llvm::FunctionCallee _new = Generator::instance->m_module->getOrInsertFunction("newArray",builder->getPtrTy(),
          IntegerType::getInt32Ty(builder->getContext()),IntegerType::getInt32Ty(builder->getContext()),PointerType::get(builder->getContext(),0));
     uint sizeB = 0;

     if(typeName.has_value()) {
          if(auto str = Generator::instance->m_file->findStruct(typeName.value()->mangle(),builder)) {

               sizeB = Generator::instance->m_module->getDataLayout().getTypeSizeInBits(str.value().first) / 8;
          }else if(auto clazz = Generator::findClass(typeName.value()->mangle(),builder)) {
               sizeB = Generator::instance->m_module->getDataLayout().getTypeSizeInBits(clazz.value().first) / 8;
          }
     }else sizeB = Generator::instance->m_module->getDataLayout().getTypeSizeInBits(getType(static_cast<TokenType>(sid))) / 8;
     Value* arr = Igel::setDbg(builder,builder->CreateCall(_new,{ConstantInt::get(IntegerType::getInt32Ty(builder->getContext()),sizeB),
          ConstantInt::get(IntegerType::getInt32Ty(builder->getContext()),size.size() - 1),sizes}),pos);
     const llvm::FunctionCallee del = Generator::instance->m_module->getOrInsertFunction("free",builder->getVoidTy(),builder->getPtrTy());
     builder->CreateCall(del,sizes);
     return arr;
}

llvm::Value * Igel::dyn_Cast(llvm::IRBuilder<> *builder,BeContained* target,BeContained* srcCont,llvm::Value* val) {
     auto calle = Generator::instance->m_module->getOrInsertFunction("__dynamic_cast",
          FunctionType::get(builder->getPtrTy(),{builder->getPtrTy(),builder->getPtrTy(),builder->getPtrTy(),builder->getInt64Ty()},false));

     if(!srcCont) {
          std::cerr << "Invalid Cast" << std::endl;
          exit(EXIT_FAILURE);
     }
     GlobalVariable* src = nullptr;
     if(auto clazz = dynamic_cast<Class*>(srcCont))src = clazz->getClassInfos(builder).typeInfo;
     if(auto intf = dynamic_cast<Interface*>(srcCont))src = intf->getClassInfos(builder).typeInfo;
     if(!src) {
          std::cerr << "Can only cast to Interfaces and Classes" << std::endl;
          exit(EXIT_FAILURE);
     }
     Generator::setTypeNameRet(target);

     if(auto clazz = dynamic_cast<Class*>(target))return builder->CreateCall(calle,{val,src,clazz->getClassInfos(builder).typeInfo,ConstantInt::get(builder->getInt64Ty(),0)});
     if(auto intf = dynamic_cast<Interface*>(target))return builder->CreateCall(calle,{val,src,intf->getClassInfos(builder).typeInfo,ConstantInt::get(builder->getInt64Ty(),0)});

     std::cerr << "Can only cast to Interfaces and Classes" << std::endl;
     exit(EXIT_FAILURE);
}

llvm::Value* Igel::stat_Cast(llvm::IRBuilder<> *builder, BeContained *target, BeContained *src, llvm::Value *val) {
     if(!dynamic_cast<ContainableType*>(src)) {
          std::cerr << "Can only cast Interfaces and Classes" << std::endl;
          exit(EXIT_FAILURE);
     }
     if(!dynamic_cast<ContainableType*>(target)) {
          std::cerr << "Can only cast to Interfaces and Classes" << std::endl;
          exit(EXIT_FAILURE);
     }
     if(target && dynamic_cast<PolymorphicType*>(src)->isSubTypeOf(dynamic_cast<ContainableType*>(target))) {
          uint offset = dynamic_cast<PolymorphicType*>(src)->getSuperOffset(dynamic_cast<ContainableType*>(target));
          if(((int) offset) == -1) {
               std::cerr << "Internal error, please report" << std::endl;
               exit(EXIT_FAILURE);
          }
          offset /= 8;
          return builder->CreateGEP(builder->getPtrTy(),val,ConstantInt::get(builder->getInt8Ty(),offset));
     }
     return nullptr;
}

llvm::Instruction * Igel::setDbg(llvm::IRBuilder<> *builder, llvm::Instruction *inst, Position pos) {
     if(!Generator::instance->debug)return inst;
     if(pos.line == -1 || pos._char == -1)Igel::internalErr("Usage of uninitialized Position");
     Debug dbg = Generator::instance->getDebug();
     DIScope* scope = Generator::instance->dbgScopes.empty()?dbg.file:Generator::instance->dbgScopes.back();
     inst->setDebugLoc(DILocation::get(builder->getContext(),pos.line,pos._char,scope));

     return inst;
}

void Igel::check_Pointer(llvm::IRBuilder<> *builder,llvm::Value *ptr) {
    if(Generator::instance->no_ptr_check)return;
    if(!ptr)return;

    Value* cmp = builder->CreateCmp(CmpInst::ICMP_EQ,ptr,ConstantPointerNull::get(builder->getPtrTy()));
    llvm::Function* parFunc = builder->GetInsertBlock()->getParent();
    llvm::BasicBlock* err = llvm::BasicBlock::Create(builder->getContext(),"then",parFunc);
    llvm::BasicBlock* notErr = llvm::BasicBlock::Create(builder->getContext(),"ifcont",parFunc);
    builder->CreateCondBr(cmp,err,notErr);
    builder->SetInsertPoint(err);
    Igel::GeneratedException::exception(builder,"Null pointer Exception",{});
    builder->SetInsertPoint(notErr);
}

void Igel::check_Arr(llvm::IRBuilder<> *builder,llvm::Value *ptr, llvm::Value *idx) {
    if(Generator::instance->no_arr_check)return;

    Value* val = builder->CreateInBoundsGEP(ptr->getType(),ptr,builder->getInt64(0));
    val = builder->CreateLoad(builder->getInt64Ty(),val);
    Value* cmp = builder->CreateCmp(CmpInst::ICMP_ULE,val,builder->CreateZExt(idx,val->getType()));
    llvm::Function* parFunc = builder->GetInsertBlock()->getParent();
    llvm::BasicBlock* err = llvm::BasicBlock::Create(builder->getContext(),"then",parFunc);
    llvm::BasicBlock* notErr = llvm::BasicBlock::Create(builder->getContext(),"ifcont",parFunc);
    builder->CreateCondBr(cmp,err,notErr);
    builder->SetInsertPoint(err);
    Igel::GeneratedException::exception(builder,"Array index out of Bounds exception: tried to access element %d of %d elements",{builder->CreateZExt(idx,val->getType()),val});
    builder->SetInsertPoint(notErr);
}

llvm::Value *Igel::getString(std::string str,llvm::IRBuilder<> *builder) {
    return builder->CreateGlobalString(str);
}

llvm::Value * NodeTermCast::generate(llvm::IRBuilder<> *builder) {
     Generator::clearTypeNameRet();
     Value* val = expr->generate(builder);
     if(!Generator::typeNameRet) {
          Igel::err("Invalid expression used for casting",pos);
     }
     auto typeName = Generator::typeNameRet;
     if(!dynamic_cast<ContainableType*>(typeName->type)) {
          Igel::err("Can only cast Interfaces and Classes",pos);
     }
     if(!dynamic_cast<ContainableType*>(target->type)) {
          Igel::err("Can only cast to Interfaces and Classes",pos);
     }

     Generator::setTypeNameRet(target);
     if(!std::equal(target->templateTypes.begin(), target->templateTypes.end(),typeName->templateTypes.begin(),typeName->templateTypes.end())){
         Igel::err("Template Values of Casted types do not match",expr->pos);
     }
     if(auto cast = Igel::stat_Cast(builder,target->type,typeName->type,val))return cast;
     llvm::Function* parFunc = builder->GetInsertBlock()->getParent();
     llvm::BasicBlock* err = llvm::BasicBlock::Create(builder->getContext(),"then",parFunc);
     llvm::BasicBlock* notErr = llvm::BasicBlock::Create(builder->getContext(),"ifcont",parFunc);
     Value* casted = Igel::dyn_Cast(builder,target->type,typeName->type,val);
     Value* cmp = builder->CreateCmp(CmpInst::ICMP_EQ,casted,ConstantPointerNull::get(builder->getPtrTy()));
     builder->CreateCondBr(cmp,err,notErr);

     builder->SetInsertPoint(err);
     Igel::GeneratedException::exception(builder,"ClassCastException: Cannot cast: %s to %s",{Igel::getString(typeName->type->mangle(),builder),Igel::getString(target->type->mangle(),builder)});
     builder->CreateUnreachable();
     notErr->moveAfter(err);
     builder->SetInsertPoint(notErr);
     return casted;
}

llvm::Value * NodeTermCast::generatePointer(llvm::IRBuilder<> *builder) {
     Igel::internalErr("Cannot generate Pointer to cast");
     return nullptr;
}

llvm::Value * NodeTermInstanceOf::generate(llvm::IRBuilder<> *builder) {
    //TODO CASTING FOR TEMPLATE TYPES
     Generator::clearTypeNameRet();
     Value* val = expr->generate(builder);
     if(!Generator::typeNameRet) {
          Igel::err("Invalid expression used for instanceOf",pos);
     }
     auto typeName = Generator::typeNameRet;
     if(!dynamic_cast<ContainableType*>(typeName->type)) {
          Igel::err("Can only cast Interfaces and Classes",pos);
     }
     if(!dynamic_cast<ContainableType*>(target)) {
          Igel::err("Can only cast to Interfaces and Classes",pos);
     }
     Generator::setTypeNameRet(target);
     if(auto cast = Igel::stat_Cast(builder,target,typeName->type,val)) return ConstantInt::get(builder->getInt1Ty(),1);
     Value* casted = Igel::dyn_Cast(builder,target,typeName->type,val);
     return builder->CreateCmp(CmpInst::ICMP_NE,casted,ConstantPointerNull::get(builder->getPtrTy()));
}

llvm::Value * NodeTermInstanceOf::generatePointer(llvm::IRBuilder<> *builder) {
     Igel::internalErr("Cannot generate Pointer to cast");
}

llvm::Value * NodeTermInlineIf::generate(llvm::IRBuilder<> *builder) {
     Value* cmp = cond->generate(builder);

     Value* lsV = ls->generate(builder);
     Value* lsR = rs->generate(builder);
     return builder->CreateSelect(cmp,lsV,lsR);
}

std::string Name::mangle() {
     return Igel::Mangler::mangleName(this);
}

llvm::Type* getType(llvm::Type* type) {
     return type == reinterpret_cast<Type *>(2) ?PointerType::get(Generator::instance->m_module->getContext(),0):type;
}

llvm::Value* generateS(std::variant<NodeStmtScope*,NodeStmt*> var,llvm::IRBuilder<>* builder) {
     static llvm::Value* val;
     struct Visitor {
          llvm::IRBuilder<>* builder;
          void operator()(NodeStmt* stmt) const {
               val = stmt->generate(builder);
          }
          void operator()(NodeStmtScope* scope) const {
               val = scope->generate(builder);
          }
     };

     Visitor visitor{.builder = builder};
     std::visit(visitor,var);
     return val;
}