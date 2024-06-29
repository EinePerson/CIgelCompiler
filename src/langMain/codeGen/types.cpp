//
// Created by igel on 19.11.23.
//

#include <llvm/IR/Verifier.h>
#include <llvm/Support/CommandLine.h>
#include <llvm/Support/JSON.h>

#include "Generator.h"
#include "../../util/Mangler.h"

//Name for the specifier of an array length
#define ARR_LENGTH "length"

llvm::Value* NodeTermId::generate(llvm::IRBuilder<>* builder) {
     if(contained) {
          return contained.value()->generate(builder);
     }
     Var* var = Generator::instance->getVar(Igel::Mangler::mangleName(cont));
     if(auto str = dynamic_cast<StructVar*>(var)) {
          Generator::typeNameRet = str->str;
          Generator::structRet = str->str;
          Generator::classRet = nullptr;
     }
     _signed = var->_signed;
     if(auto clazz = dynamic_cast<ClassVar*>(var)) {
          Generator::typeNameRet = clazz->clazz;
     }
     return builder->CreateLoad(var->getType(),var->alloc);
}

llvm::Value* NodeTermId::generatePointer(llvm::IRBuilder<>* builder) {
     if(contained) {
          return contained.value()->generatePointer(builder);
     }
     Var* var = Generator::instance->getVar(Igel::Mangler::mangleName(cont));
     if(auto str = dynamic_cast<StructVar*>(var)){
          Generator::typeNameRet = str->str;
          Generator::structRet = str->str;
          Generator::classRet = nullptr;
     }
     //if(auto arr = dynamic_cast<ArrayVar*>(var))Generator::typeNameRet.
     Value* ptr = var->alloc;
     if(auto clazz = dynamic_cast<ClassVar*>(var)) {
          Generator::typeNameRet = clazz->clazz;
     }
     return ptr;
}

llvm::Value * NodeTermAcces::generate(llvm::IRBuilder<> *builder) {
     Generator::arrRet = false;
     if(call) {
          Value* val = call->generate(builder);
          if(auto func = Generator::instance->m_file->findIgFunc(call->name->mangle(),call->params)) {
               auto clz = Generator::instance->m_file->findClass(func.value()->retTypeName);
               auto str = Generator::instance->m_file->findStruct(func.value()->retTypeName);
               if(!clz || !str)return nullptr;

               if(clz.has_value())Generator::typeNameRet = clz.value().second;
               else Generator::typeNameRet = str.value().second;

               if(acc.value.value() == ARR_LENGTH && Generator::arrRet) {
                    return  builder->CreateStructGEP(clz.has_value()?clz.value().first:str.value().first,val,0);
               }
               uint fid = clz.value().second?clz->second->varIdMs[acc.value.value()]:str->second->varIdMs[acc.value.value()];
               return  builder->CreateLoad(clz.has_value()?clz.value().second->types[fid]:str.value().second->types[fid],this->generatePointer(builder));

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
               Generator::structRet = nullptr;
               isClz = false;
          }else {
               Generator::contained = true;
               val = contained.value()->generate(builder);
               clz = Generator::classRet;
               Generator::classRet = nullptr;
               isClz = true;
          }
          Generator::contained = false;
     }else {
          if(var = dynamic_cast<StructVar*>(Generator::instance->getVar(id->mangle()))) {
               val = var->alloc;
               isClz = false;
          }else if(clzVar = dynamic_cast<ClassVar*>(Generator::instance->getVar(id->mangle()))){
               val = builder->CreateLoad(clzVar->type,clzVar->alloc);
               isClz = true;
          }else {
               auto arrVar = dynamic_cast<ArrayVar*>(Generator::instance->getVar(id->mangle()));
               if(!arrVar || acc.value.value() != ARR_LENGTH) {
                    std::cerr << "Expected array length" << std::endl;
                    exit(EXIT_FAILURE);
               }

               val = arrVar->alloc;
               if(arrVar->typeName.has_value() && Generator::instance->m_file->findClass(arrVar->typeName.value()->mangle()))val = builder->CreateLoad(builder->getPtrTy(),val);
               val = builder->CreateInBoundsGEP(val->getType(),val,builder->getInt64(0));
               return builder->CreateLoad(builder->getInt64Ty(),val);
          }
          //TODO add lenght for Array Var
     }

     if(!(str || var || clz || clzVar)) {
          std::cerr << "Neither contained value nor variable" << std::endl;
          exit(EXIT_FAILURE);
     }

     if(!isClz) {
          uint fid = var ? var->vars[acc.value.value()]:str->varIdMs[acc.value.value()];
          if(acc.value.value() == ARR_LENGTH && Generator::arrRet)fid = 0;
          Value* ptr = builder->CreateStructGEP(var?var->strType:str->strType,val,fid);
          Generator::typeNameRet = str?str:var->str;
          if(var) {
               Generator::structRet = var->str;
               Generator::classRet = nullptr;
               return builder->CreateLoad(var->types[fid],ptr);
          }

          if(str->typeName[fid] != nullptr) {
               if(const auto strT = Generator::instance->m_file->findStruct(str->typeName[fid]->mangle())) {
                    Generator::structRet = strT.value().second;
                    Generator::classRet = nullptr;
               }
               else Generator::structRet = nullptr;
          }else Generator::structRet = nullptr;
          return ptr;
     }else {
          Value* ptr = nullptr;
          if(!(clzVar ? clzVar->clazz->varIdMs.contains(acc.value.value()):clz->varIdMs.contains(acc.value.value()))) {
               std::cerr << "Unkown variable name: " << acc.value.value();
               Igel::errAt(acc);
          }
          uint fid = clz?clz->varIdMs[acc.value.value()]:clzVar->clazz->varIdMs[acc.value.value()];
          if(acc.value.value() == ARR_LENGTH && Generator::arrRet)fid = 0;
          ptr = builder->CreateStructGEP(clzVar?clzVar->strType:clz->strType,val,fid);
          Generator::typeNameRet = clzVar?clzVar->clazz:clz;
          if(Generator::contained) {
               if(const auto strT = Generator::instance->m_file->findClass(clz?clz->vars[fid - clz->vTablePtrs]->typeName->mangle():
                    clzVar->clazz->vars[fid - clzVar->clazz->vTablePtrs]->typeName->mangle())) {
                    Generator::classRet = strT.value().second;
                    Generator::structRet = nullptr;
               }
               Generator::contained = false;
          }else {
               Generator::classRet = clz?clz :clzVar->clazz;
               Generator::structRet = nullptr;
          }
          /*if(!(clzVar ? clzVar->vars.contains(acc.value.value()):clz->varIdMs.contains(acc.value.value()))) {
               std::cerr << "Unkown variable name: " << acc.value.value();
               Igel::errAt(acc);
          }
          uint fid = clzVar ? clzVar->vars[acc.value.value()]:clz->varIdMs[acc.value.value()];
          if(acc.value.value() == ARR_LENGTH && Generator::arrRet)fid = 0;
          Generator::arrRet = false;
          Value* ptr = builder->CreateStructGEP(clzVar?clzVar->strType:clz->strType,val,fid);
          Generator::typeNameRet = clz?clz:clzVar->clazz;
          if(Generator::contained) {
               if(const auto strT = Generator::instance->m_file->findClass(clz?clz->vars[fid - 1]->typeName->mangle():clzVar->clazz->vars[fid - 1]->typeName->mangle())) {
                    Generator::classRet = strT.value().second;
                    Generator::structRet = nullptr;
               }
               Generator::contained = false;
          }else {
               Generator::classRet = clz?clz :clzVar->clazz;
               Generator::structRet = nullptr;
          }*/
          /*return ptr;
          if(clzVar) {
               Generator::classRet = clzVar->clazz;
               Generator::structRet = nullptr;
               return builder->CreateLoad(clzVar->types[fid],ptr);
          }

          if(clz->typeName[fid - 1] != nullptr) {
               if(const auto strT = Generator::instance->m_file->findClass(clz->typeName[fid - 1]->mangle())) {
                    Generator::classRet = strT.value().second;
                    Generator::structRet = nullptr;
               }
               else Generator::classRet = nullptr;
          }else Generator::classRet = nullptr;*/
          //fid = clz?clz->varIdMs[acc.value.value()]:clzVar->clazz->varIdMs[acc.value.value()];
          return builder->CreateLoad(clz?clz->types[fid]:clzVar->clazz->types[fid],ptr);
     }
}

llvm::Value * NodeTermAcces::generatePointer(llvm::IRBuilder<> *builder) {
     Generator::arrRet = false;
     if(call) {
          Value* val = call->generate(builder);
          if(auto func = Generator::instance->m_file->findIgFunc(call->name->mangle(),call->params)) {
               auto clz = Generator::instance->m_file->findClass(func.value()->retTypeName);
               auto str = Generator::instance->m_file->findStruct(func.value()->retTypeName);
               if(!clz || !str)return nullptr;

               if(clz.has_value())Generator::typeNameRet = clz.value().second;
               else Generator::typeNameRet = str.value().second;

               if(acc.value.value() == ARR_LENGTH && Generator::arrRet) {
                    return  builder->CreateStructGEP(clz.has_value()?clz.value().first:str.value().first,val,0);
               }
               uint fid = clz.value().second?clz->second->varIdMs[acc.value.value()]:str->second->varIdMs[acc.value.value()];
               return  builder->CreateStructGEP(clz.has_value()?clz.value().first:str.value().first,val,fid);

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
               Generator::structRet = nullptr;
               isClz = false;
          }else {
               Generator::contained = true;
               val = contained.value()->generate(builder);
               clz = Generator::classRet;
               Generator::classRet = nullptr;
               isClz = true;
          }
          Generator::contained = false;
     }else {
          if(var = dynamic_cast<StructVar*>(Generator::instance->getVar(id->mangle()))) {
               val = var->alloc;
               isClz = false;
          }else {
               auto tst = Generator::instance->getVar(id->mangle());
               clzVar = dynamic_cast<ClassVar*>(Generator::instance->getVar(id->mangle()));
               val = builder->CreateLoad(clzVar->type,clzVar->alloc);
               isClz = true;
          }
     }

     if(!(str || var || clz || clzVar)) {
          std::cerr << "Neither contained value nor variable" << std::endl;
          exit(EXIT_FAILURE);
     }

     if(!isClz) {
          //TODO add error when variable name is not contained in the struct
          uint fid = var ? var->vars[acc.value.value()]:str->varIdMs[acc.value.value()];
          if(acc.value.value() == ARR_LENGTH && Generator::arrRet)fid = 0;
          Value* ptr = builder->CreateStructGEP(var?var->strType:str->strType,val,fid);
          Generator::typeNameRet = str?str:var->str;
          if(var) {
               Generator::structRet = var->str;
               return ptr;
          }

          if(str->typeName[fid] != nullptr) {
               if(const auto strT = Generator::instance->m_file->findStruct(str->typeName[fid]->mangle())) {
                    Generator::structRet = strT.value().second;
                    Generator::classRet = nullptr;
               }
               else Generator::structRet = nullptr;
          }else Generator::structRet = nullptr;
          return ptr;
     }else {
          uint fid;
          Value* ptr = nullptr;
          if(!Generator::stump) {
               if(!(clzVar ? clzVar->clazz->varIdMs.contains(acc.value.value()):clz->varIdMs.contains(acc.value.value()))) {
                    std::cerr << "Unkown variable name: " << acc.value.value();
                    Igel::errAt(acc);
               }
               fid = clz?clz->varIdMs[acc.value.value()]:clzVar->clazz->varIdMs[acc.value.value()];
               if(acc.value.value() == ARR_LENGTH && Generator::arrRet)fid = 0;
               ptr = builder->CreateStructGEP(clzVar?clzVar->strType:clz->strType,val,fid);
          }
          //fid = clzVar ? clzVar->vars[acc.value.value()]:clz->varIdMs[acc.value.value()];
          Generator::typeNameRet = clzVar?clzVar->clazz:clz;
          if(Generator::contained) {
               if(const auto strT = Generator::instance->m_file->findClass(clz?clz->vars[fid - clz->vTablePtrs]->typeName->mangle():
                    clzVar->clazz->vars[fid - clzVar->clazz->vTablePtrs]->typeName->mangle())) {
                    Generator::classRet = strT.value().second;
                    Generator::structRet = nullptr;
               }
               Generator::contained = false;
          }else {
               Generator::classRet = clz?clz :clzVar->clazz;
               Generator::structRet = nullptr;
          }
          Generator::stump = false;
          return ptr;
          /*if(clzVar) {

               return ptr;
          }*/

          /*if(clz->typeName[fid - 1] != nullptr) {
               if(const auto strT = Generator::instance->m_file->findClass(clz->typeName[fid - 1]->mangle())) {
                    Generator::classRet = strT.value().second;
                    Generator::structRet = nullptr;
               }
               else Generator::classRet = nullptr;
          }else Generator::classRet = nullptr;*/
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
               if(auto strRet = Generator::instance->m_file->findStruct(str->typeName[fid]->mangle())) {
                    Generator::typeNameRet = strRet.value().second;
                    Generator::structRet = strRet.value().second;
                    return builder->CreateLoad(str->types[fid],generatePointer(builder));
               }
               return nullptr;
          }
     }
     auto* var = static_cast<ArrayVar*>(Generator::instance->getVar(Igel::Mangler::mangleName(cont)));
     _signed = var->_signed;
     return builder->CreateLoad(var->size == exprs.size()?var->type:builder->getPtrTy(),generatePointer(builder));
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
                    if(auto strRet = Generator::instance->m_file->findStruct(var->typeName.value()->mangle())) {
                         Generator::structRet = strRet.value().second;
                         Generator::classRet = nullptr;
                    }
                    else if(auto clazzRet = Generator::instance->m_file->findClass(var->typeName.value()->mangle())) {
                         Generator::classRet = clazzRet.value().second;
                         Generator::structRet = nullptr;
                    }
               }
          }
     }
     if(!ptr && !var) {
          std::cerr << "Neither contained value nor variable" << std::endl;
          exit(EXIT_FAILURE);
     }
     uint i = 0;

     Type* ty;
     if(llvm::AllocaInst::classof(var->alloc))ty = static_cast<AllocaInst*>(var->alloc)->getAllocatedType();
     else if(GlobalVariable::classof(var->alloc))ty = static_cast<GlobalVariable*>(var->alloc)->getValueType();
     while (i < exprs.size() - 1 && exprs.size() >= 1) {
          Value* val = builder->CreateLoad(ptr?ptr->getType():ty,ptr?ptr:var->alloc);
          Value* idx = builder->CreateAdd(builder->getInt64(1),exprs[i]->generate(builder));
          idx = builder->CreateMul(idx,builder->getInt64(8));
          ptr = builder->CreateInBoundsGEP(builder->getInt8Ty(),val,idx);
          i++;
     }

     Value* val = builder->CreateLoad(ptr?ptr->getType():ty,ptr?ptr:var->alloc);
     Value* expr = exprs[i]->generate(builder);
     Value* idx = builder->CreateMul(builder->getInt64(Generator::instance->m_module->getDataLayout().getTypeSizeInBits(expr->getType()) / 8),expr);
     idx = builder->CreateAdd(builder->getInt64(8),idx);
     ptr = builder->CreateInBoundsGEP(builder->getInt8Ty(),val,idx);

     return ptr;
     /*Value* ep = var->alloc;
     Value* _if = nullptr;
     for (size_t i = 0;i < exprs.size();i++) {
          Value* size = exprs[i]->generate(builder);
          ep = builder->CreateGEP(var->types[var->types.size() - i - 1],ep,size);
          /*Value* arrSize = builder->CreateLoad(IntegerType::getInt32Ty(builder->getContext()),var->sizes[i]);
          if(i == 0) {
               _if = builder->CreateCmp(CmpInst::ICMP_UGE,size,arrSize);
          }else {
               _if = builder->CreateLogicalOr(_if,builder->CreateCmp(CmpInst::ICMP_UGE,size,arrSize));
          }
     }
     /*llvm::Function* parFunc = builder->GetInsertBlock()->getParent();
     llvm::BasicBlock* then = llvm::BasicBlock::Create(builder->getContext(),"then",parFunc);
     llvm::BasicBlock* elseB = llvm::BasicBlock::Create(builder->getContext(),"else");
     llvm::BasicBlock* merge = llvm::BasicBlock::Create(builder->getContext(),"ifcont");

     builder->CreateCondBr(_if,then,elseB);

     builder->SetInsertPoint(then);
     //TODO add exception throwing when IndexOutOfBounds

     builder->CreateBr(merge);
     then = builder->GetInsertBlock();

     parFunc->insert(parFunc->end(),elseB);
     builder->SetInsertPoint(elseB);
     builder->CreateBr(merge);
     elseB = builder->GetInsertBlock();

     parFunc->insert(parFunc->end(),merge);
     builder->SetInsertPoint(merge);*/
     //return ep;
}

llvm::Value* NodeTermArrayLength::generate(llvm::IRBuilder<>* builder) {
     arr->exprs.pop_back();
     Value* val = arr->generate(builder);
     val = builder->CreateInBoundsGEP(val->getType(),val,builder->getInt64(0));
     return builder->CreateLoad(builder->getInt64Ty(),val);
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
               auto str = Generator::instance->m_file->findStruct(func.value()->retTypeName);
               if(!str)return nullptr;
               Generator::typeNameRet = str.value().second;
               auto it = std::ranges::find(str.value().second->varIds,acc.value.value());
               if(it == str.value().second->varIds.end())return nullptr;
               uint fid = it - str.value().second->varIds.begin();
               return builder->CreateLoad(str.value().second->types[fid],this->generatePointer(builder));
          }else return nullptr;
     }

     Value* val;
     Struct* str = nullptr;
     StructVar* var = nullptr;
     if(contained) {
          val = contained.value()->generate(builder);
          str = Generator::structRet;
          Generator::structRet = nullptr;
     }else {
          var = dynamic_cast<StructVar*>(Generator::instance->getVar(id->mangle()));
          val = var->alloc;//builder->CreateLoad(var->type,var->alloc);
     }
     if(!str && !var) {
          std::cerr << "Neither contained value nor variable" << std::endl;
          exit(EXIT_FAILURE);
     }

     const uint fid = var ? var->vars[acc.value.value()]:str->varIdMs[acc.value.value()];
     Value* ptr = builder->CreateStructGEP(var?var->strType:str->strType,val,fid);
     Generator::typeNameRet = str?str:var->str;
     if(var) {
          Generator::structRet = var->str;
          Generator::classRet = nullptr;
          return builder->CreateLoad(var->types[fid],ptr);
     }

     if(str->typeName[fid] != nullptr) {
          if(const auto strT = Generator::instance->m_file->findStruct(str->typeName[fid]->mangle())) {
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
               auto str = Generator::instance->m_file->findStruct(func.value()->retTypeName);
               if(!str)return nullptr;
               Generator::typeNameRet = str.value().second;
               auto it = std::ranges::find(str.value().second->varIds,acc.value.value());
               if(it == str.value().second->varIds.end())return nullptr;
               uint fid = it - str.value().second->varIds.begin();
               return  builder->CreateStructGEP(str.value().first,val,fid);

          }else return nullptr;
     }
     Value* val;
     Struct* str = nullptr;
     StructVar* var = nullptr;
     if(contained) {
          val = contained.value()->generatePointer(builder);
          str = Generator::structRet;
          Generator::structRet = nullptr;
     }else {
          var = dynamic_cast<StructVar*>(Generator::instance->getVar(id->mangle()));
          val = var->alloc;//builder->CreateLoad(var->type,var->alloc);
     }

     if(!str && !var) {
          std::cerr << "Neither contained value nor variable" << std::endl;
          exit(EXIT_FAILURE);
     }

     //TODO add error when variable name is not contained in the struct
     const uint fid = var ? var->vars[acc.value.value()]:str->varIdMs[acc.value.value()];
     Value* ptr = builder->CreateStructGEP(var?var->strType:str->strType,val,fid);
     Generator::typeNameRet = str?str:var->str;
     if(var) {
          Generator::structRet = var->str;
          Generator::classRet = nullptr;
          return ptr;
     }

     if(str->typeName[fid] != nullptr) {
          if(const auto strT = Generator::instance->m_file->findStruct(str->typeName[fid]->mangle())) {
               Generator::structRet = strT.value().second;
               Generator::classRet = nullptr;
          }
          else Generator::structRet = nullptr;
     }else Generator::structRet = nullptr;
     return ptr;
}

llvm::Value* NodeTermClassAcces::generate(llvm::IRBuilder<>* builder) {
     if(call) {
          call->generate(builder);
          if(auto func = Generator::instance->m_file->findIgFunc(call->name->mangle(),call->params)) {
               auto str = Generator::instance->m_file->findClass(func.value()->retTypeName);
               if(!str)return nullptr;
               Generator::typeNameRet = str.value().second;
               uint fid = str.value().second->varIdMs[acc.value.value()];
               return builder->CreateLoad(str.value().second->types[fid],this->generatePointer(builder));
          }else return nullptr;
     }

     Value* val;
     Class* str = nullptr;
     ClassVar* var = nullptr;
     if(contained) {
          val = contained.value()->generate(builder);
          str = Generator::classRet;
     }else {
          var = dynamic_cast<ClassVar*>(Generator::instance->getVar(id->mangle()));
          val = builder->CreateLoad(var->type,var->alloc);
     }
     if(!str && !var) {
          std::cerr << "Neither contained value nor variable" << std::endl;
          exit(EXIT_FAILURE);
     }

     const uint fid = var ? var->vars[acc.value.value()]:str->varIdMs[acc.value.value()];
     Value* ptr = builder->CreateStructGEP(var?var->strType:str->strType,val,fid);
     Generator::typeNameRet = str?str:var->clazz;
     if(var) {
          Generator::classRet = var->clazz;
          Generator::structRet = nullptr;
          return builder->CreateLoad(var->types[fid],ptr);
     }

     if(str->typeName[fid - 1] != nullptr) {
          if(const auto strT = Generator::instance->m_file->findClass(str->typeName[fid - 1]->mangle())) {
               Generator::classRet = strT.value().second;
               Generator::structRet = nullptr;
          }
          else Generator::classRet = nullptr;
     }else Generator::classRet = nullptr;
     return builder->CreateLoad(str->types[fid - 1],ptr);
}

llvm::Value* NodeTermClassAcces::generatePointer(llvm::IRBuilder<>* builder) {
     if(call) {
          Value* val = call->generate(builder);
          if(auto func = Generator::instance->m_file->findIgFunc(call->name->mangle(),call->params)) {
               auto str = Generator::instance->m_file->findClass(func.value()->retTypeName);
               if(!str)return nullptr;
               Generator::typeNameRet = str.value().second;
               uint fid = str->second->varIdMs[acc.value.value()];
               return  builder->CreateStructGEP(str.value().first,val,fid);

          }else return nullptr;
     }
     Value* val;
     Class* str = nullptr;
     ClassVar* var = nullptr;
     if(contained) {
          val = contained.value()->generate(builder);
          str = Generator::classRet;
     }else {
          var = dynamic_cast<ClassVar*>(Generator::instance->getVar(id->mangle()));
          val = builder->CreateLoad(var->type,var->alloc);
     }

     if(!str && !var) {
          std::cerr << "Neither contained value nor variable" << std::endl;
          exit(EXIT_FAILURE);
     }

     //TODO add error when variable name is not contained in the struct
     const uint fid = var ? var->vars[acc.value.value()]:str->varIdMs[acc.value.value()];
     Value* ptr = builder->CreateStructGEP(var?var->strType:str->strType,val,fid);
     Generator::typeNameRet = str?str:var->clazz;
     if(var) {
          Generator::classRet = var->clazz;
          Generator::structRet = nullptr;
          return ptr;
     }

     if(str->typeName[fid - 1] != nullptr) {
          if(const auto strT = Generator::instance->m_file->findClass(str->typeName[fid - 1]->mangle())) {
               Generator::classRet = strT.value().second;
               Generator::structRet = nullptr;
          }
          else Generator::classRet = nullptr;
     }else Generator::classRet = nullptr;
     return ptr;
}

llvm::Value* NodeTermAcces::generateClassPointer(llvm::IRBuilder<>* builder) {
     if(call) {
          Value* val = call->generate(builder);
          if(auto func = Generator::instance->m_file->findIgFunc(call->name->mangle(),call->params)) {
               auto str = Generator::instance->m_file->findClass(func.value()->retTypeName);
               if(!str)return nullptr;
               Generator::typeNameRet = str.value().second;
               uint fid = str->second->varIdMs[acc.value.value()];
               return  val;

          }
     }

     Value* val;
     Class* str = nullptr;
     ClassVar* var = nullptr;
     if(contained) {
          val = contained.value()->generatePointer(builder);
          str = Generator::classRet;
     }else {
          var = dynamic_cast<ClassVar*>(Generator::instance->getVar(id->mangle()));
          val = builder->CreateLoad(var->type,var->alloc);
     }

     if(!str && !var) {
          std::cerr << "Neither contained value nor variable" << std::endl;
          exit(EXIT_FAILURE);
     }

     //TODO add error when variable name is not contained in the struct
     const uint fid = var ? var->vars[acc.value.value()]:str->varIdMs[acc.value.value()];
     Generator::typeNameRet = str?str:var->clazz;
     if(var) {
          Generator::classRet = var->clazz;
          Generator::structRet = nullptr;
          return val;
     }

     if(str->typeName[fid] != nullptr) {
          if(const auto strT = Generator::instance->m_file->findClass(str->typeName[fid]->mangle())) {
               Generator::classRet = strT.value().second;
               Generator::structRet = nullptr;
          }
          else Generator::classRet = nullptr;
     }else Generator::classRet = nullptr;
     return val;
}

llvm::Value * NodeStmtSuperConstructorCall::generate(llvm::IRBuilder<> *builder) {
     if(!_this->extending.has_value()) {
          std::cerr << "Call to super constructor can only be done in class with super at " << _this->mangle() << std::endl;
          exit(EXIT_FAILURE);
     }
     std::vector<Type*> types {builder->getPtrTy()};
     std::vector<BeContained*> names {new Name("")};
     std::vector<bool> sing = {true};

     std::vector<Value*> params {};
     params.reserve(exprs.size());

     for (auto expr : exprs) {
          auto exprRet = expr->generate(builder);
          params.push_back(exprRet);
          Generator::typeNameRet = nullptr;
          types.push_back(exprRet->getType());
          names.push_back(Generator::typeNameRet);
          sing.push_back(expr->_signed);
          Generator::typeNameRet = nullptr;
     }

     FunctionType* type = FunctionType::get(PointerType::get(builder->getContext(),0),{Type::getInt64Ty(builder->getContext())},false);
     ArrayRef<Type*> llvmTypes {types};
     auto calle = Generator::instance->m_module->getOrInsertFunction(Igel::Mangler::mangle(_this->extending.value(),types,names,sing,true),builder->getVoidTy(),llvmTypes);

     Var* ptrA = Generator::instance->getVar("_Z4this");
     Value* ptrL = builder->CreateLoad(ptrA->getType(),ptrA->alloc);
     params.insert(params.begin(),ptrL);
     //for (auto expr : exprs) params.push_back(expr->generate(builder));

     Value* ptr = builder->CreateCall(calle,params);
     Generator::typeNameRet = _this->extending.value();
     return ptr;
}

llvm::Value* NodeStmtLet::generate(llvm::IRBuilder<>* builder) {
     if(_static) {
          GlobalValue* val = new GlobalVariable(*Generator::instance->m_module,type?type:builder->getPtrTy(),final,GlobalValue::ExternalLinkage,
               type?ConstantInt::get(type,0):ConstantPointerNull::get(PointerType::get(*Generator::m_contxt,0)),mangle());
          std::pair<llvm::Value*,Var*> var = generateImpl(builder);
          var.second->alloc = val;
          Generator::instance->createStaticVar(mangle(),var.first,var.second);
          //builder->CreateStore(var.first,val);
          return val;
     }
     std::pair<llvm::Value*,Var*> var = generateImpl(builder);
     Generator::instance->createVar(mangle(),var.second);
     return var.first;
}

std::pair<llvm::Value*, Var*> NodeStmtPirimitiv::generateImpl(llvm::IRBuilder<>* builder) {
     Value* val = expr.has_value()?expr.value()->generate(builder) : ConstantInt::get(Generator::getType(static_cast<TokenType>(sid)),0);

     //Generator::instance->createVar(mangle(),Generator::getType(static_cast<TokenType>(sid)),val,sid <= 3);
     AllocaInst* alloc = nullptr;
     if(builder->GetInsertBlock()) {
          alloc = builder->CreateAlloca(type);
          builder->CreateStore(val,alloc);
     }
     return {val,new Var{alloc,_signed}};
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

std::pair<llvm::Value*, Var*> NodeStmtStructNew::generateImpl(llvm::IRBuilder<>* builder) {
     if(auto structT = Generator::instance->m_file->findStruct(typeName->mangle())) {
          AllocaInst* alloc = builder->CreateAlloca(structT.value().first);
          //Value* gen = term->generate(builder);
          //builder->CreateStore(gen,alloc);

          auto var = new StructVar(alloc);
          var->strType = structT.value().first;
          var->str = structT.value().second;
          var->types = structT.value().second->types;

          for(size_t i = 0;i < structT.value().second->vars.size();i++) {
               NodeStmtLet* let = structT.value().second->vars[i];
               std::string str = structT.value().second->vars[i]->name;
               Value* val = builder->CreateStructGEP(structT.value().first,alloc,i);
               Value* gen = structT.value().second->vars[i]->generate(builder);
               builder->CreateStore(gen,val);
               if(auto pirim = dynamic_cast<NodeStmtPirimitiv*>(let))var->signage.push_back(pirim->sid <= (char) TokenType::_long);
               else var->signage.push_back(false);
               var->vars[str] = i;
          }
          if(structT.value().second->varIdMs.empty())structT.value().second->varIdMs = var->vars;
          if(!structT.value().second->strType)structT.value().second->strType = structT.value().first;
          return {alloc,var};
     }else {
          std::cerr << "Undeclarde Type " << typeName->mangle() << std::endl;
          exit(EXIT_FAILURE);
     }
}

std::pair<llvm::Value*, Var*> NodeStmtClassNew::generateImpl(llvm::IRBuilder<>* builder) {
     if(auto clazz = Generator::instance->m_file->findClass(typeName->mangle())) {
          auto type = PointerType::get(builder->getContext(),0);
          AllocaInst* alloc = builder->CreateAlloca(type);
          Value* gen = term->generate(builder);
          builder->CreateStore(gen,alloc);

          auto var = new ClassVar(alloc);
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
          return {gen,var};
     }else {
          std::cerr << "Undeclarde Type " << typeName->mangle() << std::endl;
          exit(EXIT_FAILURE);
     }
}

Value* createArr(std::vector<Value*> sizes,std::vector<Type*> types,uint i,llvm::IRBuilder<>* builder) {
     if(i == 0)return ConstantInt::get(types[i],0);
     const llvm::FunctionCallee _new = Generator::instance->m_module->getOrInsertFunction("_Znam",PointerType::get(builder->getContext(),0),IntegerType::getInt64Ty(builder->getContext()));
     Value* var = builder->CreateCall(_new,builder->CreateMul(sizes[i - 1],ConstantInt::get(IntegerType::getInt64Ty(builder->getContext()),12)));

     return var;
}

std::pair<llvm::Value*, Var*> NodeStmtArr::generateImpl(llvm::IRBuilder<>* builder) {
     Value* val = llvm::ConstantPointerNull::get(llvm::PointerType::get(builder->getContext(),0));
     if(term)val = term->generate(builder);
     auto* var = new ArrayVar(builder->CreateAlloca(builder->getPtrTy()),sid <= 3,typeName != nullptr?typeName:(std::optional<BeContained*>) {});
     var->size = size;
     var->type = getType(static_cast<TokenType>(sid));
     builder->CreateStore(val,var->alloc);
     //Generator::instance->m_vars.back().insert_or_assign(mangle(),var);
     //Generator::instance->createVar(mangle(),var);
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
     for (size_t i = 1;i < expr.size();i++) {
          if(exprCond[i - 1] == TokenType::_and)val = builder->CreateLogicalAnd(val,expr[i]->generate(builder));
          else val = builder->CreateLogicalOr(val,expr[i]->generate(builder));
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
     Value* val = id->generatePointer(builder);
     if(op == TokenType::eq) {
          return builder->CreateStore(expr->generate(builder),val);
     }else {
          Value* load = id->generate(builder); //builder->CreateLoad(val->getType(),val);
          Value* _new = nullptr;
          if(expr != nullptr)_new = expr->generate(builder);
          //load = builder->CreateLoad(/*val->getType()->isPointerTy()?val->getType():*/static_cast<AllocaInst*>(val)->getAllocatedType(),val);
          Value* res = generateReassing(load,_new,op,builder);
          return builder->CreateStore(res,val);
     }
}

llvm::Value* NodeStmtArrReassign::generate(llvm::IRBuilder<>* builder) {
     //TODO add exception throwing when IndexOutOfBounds
     Value* ep = acces->generatePointer(builder);
     if(op == TokenType::eq) {
          Value* gen = expr.value()->generate(builder);
          Value* val = builder->CreateStore(gen,ep);
          return val;
     }else {
          auto* var = static_cast<ArrayVar*>(Generator::instance->getVar(Igel::Mangler::mangleName(acces->cont)));
          Value* load = builder->CreateLoad(var->type,generatePointer(builder));
          Value* _new = expr.value()->generate(builder);
          Value* res = generateReassing(load,_new,op,builder);
          return builder->CreateStore(res,ep);
     }
};

llvm::Value* NodeStmtReturn::generate(llvm::IRBuilder<>* builder)  {
     if(val) {
          return  builder->CreateRet(val.value()->generate(builder));
     }
     return  builder->CreateRetVoid();
};

llvm::Value* NodeStmtBreak::generate(llvm::IRBuilder<>* builder) {
     Value* val = builder->CreateBr(Generator::instance->after.back());
     Generator::lastUnreachable = true;
     return val;
}

llvm::Value* NodeStmtContinue::generate(llvm::IRBuilder<>* builder) {
     Value* val = builder->CreateBr(Generator::instance->next.back());
     Generator::lastUnreachable = true;
     return val;
}

llvm::Value * NodeStmtThrow::generate(llvm::IRBuilder<> *builder) {
     llvm::FunctionCallee cxa_Throw = Generator::instance->m_module->getOrInsertFunction("__cxa_throw",FunctionType::get(builder->getVoidTy(),
          {builder->getPtrTy(),builder->getPtrTy(),builder->getPtrTy()},false));
     llvm::FunctionCallee cxa_alloc = Generator::instance->m_module->getOrInsertFunction("__cxa_allocate_exception",FunctionType::get(builder->getPtrTy(),builder->getInt64Ty(),false));
     llvm::FunctionCallee cpy = Generator::instance->m_module->getOrInsertFunction("memcpy",FunctionType::get(builder->getVoidTy(),
          {builder->getPtrTy(),builder->getPtrTy(),builder->getInt64Ty()},false));

     Generator::typeNameRet = nullptr;
     Value* val = expr->generatePointer(builder);
     std::optional<std::pair<StructType*,Class*>> clazz = Generator::instance->m_file->findClass(Generator::typeNameRet->mangle());
     if(!clazz.has_value()) {
          std::cerr << "Could not find class " << Generator::typeNameRet << std::endl;
          exit(1);
     }
     if(!Generator::instance->unreach) {
          Generator::instance->unreach = BasicBlock::Create(builder->getContext(),"unreachable",builder->GetInsertBlock()->getParent());
     }
     Generator::lastUnreachable = true;
     Value* excep = builder->CreateCall(cxa_alloc,builder->getInt64(8));
     builder->CreateStore(val,excep);
     //builder->CreateCall(cpy,{excep,expr->generatePointer(builder),builder->getInt64(Generator::instance->m_module->getDataLayout().getTypeSizeInBits(clazz.value().first) / 8)});
     if(Generator::catches.empty()) {
          builder->CreateCall(cxa_Throw,{excep,clazz.value().second->typeInfoPointVar,ConstantPointerNull::get(builder->getPtrTy())});
          return  builder->CreateUnreachable();
     }else return builder->CreateInvoke(cxa_Throw,Generator::instance->unreach,Generator::catches.back(),{excep,clazz.value().second->typeInfoPointVar,ConstantPointerNull::get(builder->getPtrTy())});
}

llvm::Value * NodeStmtCatch::generate(llvm::IRBuilder<> *builder) {
     std::cerr << "Cannot generate a Catch directly use NodeStmtTry" << std::endl;
     exit(EXIT_FAILURE);
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
     BasicBlock* cont = nullptr; /*= BasicBlock::Create(builder->getContext(), "cont", func);*/
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

          auto clazz = Generator::instance->m_file->findClass(i->typeName->mangle());
          Value* varRet = builder->CreateCall(begin_Catch,{exP});
          AllocaInst* alloc = builder->CreateAlloca(builder->getPtrTy());
          Value* varLoad = builder->CreateLoad(builder->getPtrTy(),varRet);
          builder->CreateStore(varLoad,alloc);
          auto var = new ClassVar(alloc);
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
          land->addClause(clazz->second->typeInfoPointVar);
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

               std::optional<std::pair<StructType*,Class*>> clazz = Generator::instance->m_file->findClass(catch_[i]->typeName->mangle());
               if(!clazz.has_value()) {
                    std::cerr << "Could not find class " << catch_[i]->typeName << std::endl;
                    exit(1);
               }
               Value* ctId = builder->CreateCall(typeIdC,clazz.value().second->typeInfo);
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

std::string IgFunction::mangle() {
     return Igel::Mangler::mangle(this,paramType,paramTypeName,signage,member,constructor);
}

int Types::getSize(TokenType type) {
     if(type > TokenType::_ulong)return -1;
     const int i = static_cast<int>(type) % 4;
     return  (i + 1) * 8 + 8 * std::max(i - 1,0) + 16 * std::max(i - 2,0);
}

void Struct::generateSig(llvm::IRBuilder<>* builder) {
     for(uint i = 0; i < this->types.size();i++) {
          if(types[i] == nullptr) {
               std::cerr << "Type is not set in " << mangle() << std::endl;
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
                    std::cerr << "Unkown type id: " << tI << std::endl;
                    exit(EXIT_FAILURE);
          }
     }
     Generator::instance->m_file->structs[mangle()] = std::make_pair(StructType::create(builder->getContext(),mangle()),this);
}

void Struct::generatePart(llvm::IRBuilder<> *builder) {
     StructType* type = Generator::instance->m_file->findStruct(mangle()).value().first;
     for (auto var : vars) {
          //TODO maybe mangle
          if(!var->_static)varIds.push_back(var->name);
     }
     type->setBody(types,false);
     for (auto static_var : staticVars) {
          static_var->generate(builder);
     }
}

void Struct::generate(llvm::IRBuilder<>* builder) {

}

std::string Struct::mangle() {
     return Igel::Mangler::mangleTypeName(this);
}

void NamesSpace::generateSig(llvm::IRBuilder<>* builder) {
     for(auto func : funcs)Generator::instance->genFuncSig(func);
     for(auto cont : contained) {
          if(auto type = dynamic_cast<IgType*>(cont))type->generateSig(builder);
     }
}

void NamesSpace::generate(llvm::IRBuilder<>* builder) {
     for(auto func : funcs)Generator::instance->genFunc(func);
     for(auto cont : contained) {
          if(auto type = dynamic_cast<IgType*>(cont))type->generate(builder);
     }
}

std::string Interface::mangle() {
     return Igel::Mangler::mangleTypeName(this);
}

void Class::generateSig(llvm::IRBuilder<>* builder) {
     if(wasGen)return;
     wasGen = true;
     if(extending.has_value())extending.value()->generateSig(builder);
     for(uint i = 0; i < this->types.size();i++) {
          if(types[i] == nullptr) {
               std::cerr << "Type is not set in " << mangle() << std::endl;
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
                    std::cerr << "Unkown type id: " << tI << std::endl;
               exit(EXIT_FAILURE);
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
                    std::cerr << "Same constructor definition: " << constructor->mangle() << std::endl;
                    exit(EXIT_FAILURE);
               }
          }

          FunctionType* ty = FunctionType::get(builder->getVoidTy(),{types},false);
          Function* func = Function::Create(ty,GlobalValue::ExternalLinkage,Igel::Mangler::mangle(this,types,names,sing,true),*Generator::instance->m_module);
          constructor->llvmFunc = func;
          constructor->_return = builder->getVoidTy();
          constructor->retTypeName = "";
          func->getArg(0)->setName("this");
          for(size_t i = 1;i < func->arg_size();i++) {
               if(names[i])func->getArg(i)->setName(names[i]->mangle());
               else {
                    std::string str;
                    str.push_back(Igel::Mangler::typeToChar(types[i],sing[i]));
                    func->getArg(i)->setName(str);
               }
          }
     }

     /*if(!constructor) {
          constructor = new IgFunction;
          constructor->paramTypeName = names;
          constructor->paramType = types;
          constructor->signage = sing;
          constructor->constructor = true;
     }*/



     strType = StructType::create(builder->getContext(),mangle());

     Generator::instance->m_file->classes[mangle()] = {strType,this};
}

void Class::generatePart(llvm::IRBuilder<> *builder) {
     if(partGen)return;
     partGen = true;

     auto fullFuncs = getFuncsRec();
     for(int i = 0;i < fullFuncs.size() - 1;i++)types.insert(types.begin(),builder->getPtrTy());
     if(extending.has_value())types.insert(types.begin(),extending.value()->strType);
     else types.insert(types.begin(),builder->getPtrTy());
     strType->setBody(types,false);

     vTablePtrs = fullFuncs.size();
     uint j = 0;
     for (size_t i = 0;i < vars.size();i++) {
          if(!vars[i]->_static) {
               std::string str = vars[i]->name;
               varIdMs[str] = j + fullFuncs.size();
               j++;
          }
     }
}

void Class::generate(llvm::IRBuilder<>* builder) {
     if(fullGen)return;
     fullGen = true;
     if(!partGen)generatePart(builder);
     if(extending.has_value())extending.value()->generate(builder);

     if(extending.has_value() && extending.value()->final) {
          std::cerr << "Cannot extend final class:\n" << mangle() << std::endl;
          exit(EXIT_FAILURE);
     }

     auto fullFuncs = getFuncsRec();


     for (int i = 0; i < funcs.size();i++) {
          funcs[i]->paramName.insert(funcs[i]->paramName.begin(),"");
          funcs[i]->paramType.insert(funcs[i]->paramType.begin(),builder->getPtrTy());
          funcs[i]->paramTypeName.insert(funcs[i]->paramTypeName.begin(),nullptr);
          funcs[i]->signage.insert(funcs[i]->signage.begin(),false);
          funcs[i]->member = true;
          funcs[i]->supper = this;
          if(!funcs[i]->abstract) {
               Generator::instance->genFuncSig(funcs[i]);
               Generator::instance->genFunc(funcs[i],true);
          }

          //funcIdMs[funcs[i]->name] = i;
     }

     //Function* func = Generator::instance->m_module->getFunction(Igel::Mangler::mangle(this,types,names,sing,true));
     Type* ptr = PointerType::get(*Generator::m_contxt,0);



     Generator::instance->initInfo();

     //TODO make extending and make throwables extend Exception
     typeNameVar = builder->CreateGlobalString(Igel::Mangler::mangle(this),"_ZTS" + Igel::Mangler::mangle(this),0,Generator::instance->m_module.get());
     typeNamePointVar = builder->CreateGlobalString("P" + Igel::Mangler::mangle(this),"_ZTSP" + Igel::Mangler::mangle(this),0,Generator::instance->m_module.get());
     StructType* tIT = extending.has_value()?StructType::get(*Generator::m_contxt,{builder->getPtrTy(),builder->getPtrTy(),builder->getPtrTy()}):
          StructType::get(*Generator::m_contxt,{builder->getPtrTy(),builder->getPtrTy()});
     typeInfo = new GlobalVariable(*Generator::instance->m_module,tIT,true,GlobalValue::ExternalLinkage,ConstantStruct::get(tIT,
          {ConstantPointerNull::get(builder->getPtrTy()),ConstantPointerNull::get(builder->getPtrTy()),ConstantPointerNull::get(builder->getPtrTy())}),"_ZTI" + Igel::Mangler::mangle(this));
     StructType* tIPT = StructType::get(*Generator::m_contxt,{builder->getPtrTy(),builder->getPtrTy(),builder->getInt32Ty(),builder->getPtrTy()});
     typeInfoPointVar = new GlobalVariable(*Generator::instance->m_module,tIPT,true,GlobalVariable::ExternalLinkage,ConstantStruct::get(tIPT,
          {(Constant*) builder->CreateConstGEP1_64(ptr,Generator::instance->cxx_pointer_type_info,2),typeNamePointVar,
               ConstantInt::get(builder->getInt32Ty(),0),typeInfo}),"_ZTIP" + Igel::Mangler::mangle(this));
     std::vector<Constant*> vtablefuncs {ConstantPointerNull::get(builder->getPtrTy()),typeInfo};

     //TODO insert destructors into vtable

     //Constant* vtableInit = ConstantArray::get(ArrayType::get(ptr,funcs.size() + 2 /*nullptr and type info*/ + 0 /*Destructors*/),{vtablefuncs});

     std::vector<IgFunction*> overides {};
     /*while (current) {
          for (int i = 0;i < fullFuncs.size();i++){
               if(current->funcs[i]->_override) {
                    overides.push_back(current->funcs[i]);
                    current->funcs.erase(std::ranges::remove(current->funcs,current->funcs[i]).begin(),current->funcs.end());
               }
          }
          if(current->extending.has_value())current = current->extending.value();
          else current = nullptr;
     }*/
     for (auto full_func : fullFuncs[0]) {
          if(full_func->_override) {
               overides.push_back(full_func);
               fullFuncs[0].erase(std::ranges::remove(fullFuncs[0],full_func).begin(),fullFuncs[0].end());
          }
     }

     for (auto & fullFunc : fullFuncs) {
          for (auto & j : fullFunc) {
               IgFunction* func = j;
               if(!func->abstract)continue;
               for (auto overide : overides) {
                    if(func->name != overide->name)continue;
                    j = overide;
               }
          }
     }

     std::vector<Constant*> vtables;
     //if(fullFuncs.size() > 1) {
          vtables.reserve(fullFuncs.size());
          for (int i = 0;i < fullFuncs.size();i++) {
               std::vector<Constant*> vtablefuncsimp {(Constant*) builder->CreateIntToPtr(ConstantInt::get(builder->getInt64Ty(),i * -8),builder->getPtrTy()),typeInfo};
               vtablefuncsimp.reserve(fullFuncs[i].size());
               for(int j = 0;j < fullFuncs[i].size();j++) {
                    if(!abstract) {
                         if(fullFuncs[i][j]->abstract/* || funcRec->llvmFunc == nullptr*/) {
                              std::cerr << "Abstract function has either to be declared or the class has to be declared abstract\n     in: " << mangle() << std::endl;
                              exit(EXIT_FAILURE);
                         }
                         vtablefuncsimp.push_back(fullFuncs[i][j]->llvmFunc);
                    }else {
                         vtablefuncsimp.push_back(Generator::instance->cxx_pure_virtual);
                    }
                    if(!fullFuncs[i][j]->abstract)funcIdMs[fullFuncs[i][j]->name] = std::make_pair(i,j);
                    funcMap[i][j] = fullFuncs[i][j];
               }
               vtables.push_back(ConstantArray::get(ArrayType::get(ptr,fullFuncs[i].size() + 2 /*nullptr and type info*/ + 0 /*Destructors*/),{vtablefuncsimp}));
          }
     //}

     std::vector<Type*> vtableTypes {};
     vtableTypes.reserve(vtables.size());
     for (auto vt : vtables) vtableTypes.push_back(vt->getType());

     Constant* vtableStr = ConstantStruct::get(StructType::get(builder->getContext(),vtableTypes),vtables);

     vtable = new GlobalVariable(*Generator::instance->m_module,vtableStr->getType()/*vtableInit->getType()*/,true,GlobalValue::ExternalLinkage,vtableStr/*vtableInit*/,
          "_ZTV" + Igel::Mangler::mangle(this));
     if(extending.has_value()) {
          ArrayRef<Constant*> cnt = {(Constant*) (builder->CreateConstGEP1_64(ptr, vtable /*Generator::instance->cxx_class_type_info*/, 2)),(Constant*) typeNameVar,extending.value()->typeInfo};
          typeInfo->setInitializer(ConstantStruct::get(tIT,cnt));
     }else {
          ArrayRef<Constant*> cnt = {(Constant*) (builder->CreateConstGEP1_64(ptr, /*vtable*/ Generator::instance->cxx_class_type_info, 2)),(Constant*) typeNameVar};
          typeInfo->setInitializer(ConstantStruct::get(tIT,cnt));
     }


     //TODO insert destructors into vtable
     /*vtablefuncs.reserve(funcs.size() + 0);
     for (auto func : funcs)vtablefuncs.push_back(func->llvmFunc);
     vtables.front() = ConstantArray::get(ArrayType::get(ptr,funcs.size() + 2 /*nullptr and type info*/  /*Destructors*//*),{vtablefuncs});
     vtableStr = ConstantStruct::get(StructType::get(builder->getContext(),vtableTypes),vtables);
     vtable->setInitializer(vtableStr);
     vtable->print(outs());
     vtable->getValueType()->print(outs());*/

     for (auto constructor : constructors) {
          BasicBlock* entry = BasicBlock::Create(*Generator::m_contxt,"entry",constructor->llvmFunc);
          builder->SetInsertPoint(entry);

          AllocaInst* alloc = builder->CreateAlloca(ptr);
          builder->CreateStore(constructor->llvmFunc->getArg(0),alloc);
          LoadInst* load = builder->CreateLoad(alloc->getAllocatedType(),alloc);

          Generator::instance->m_vars.emplace_back();
          constructor->llvmFunc->getArg(0)->setName("this");
          Generator::instance->createVar(constructor->llvmFunc->getArg(0),false,this->mangle());
          bool supConstCall = false;
          if(!constructor->scope->stmts.empty() && dynamic_cast<NodeStmtSuperConstructorCall*>(constructor->scope->stmts[0])) {
               constructor->scope->startIndex = 1;
               constructor->scope->stmts[0]->generate(builder);
               supConstCall = true;
          }


          if(extending.has_value() && extending.value()->defaulfConstructor.has_value() && !supConstCall)builder->CreateCall(extending.value()->defaulfConstructor.value()->llvmFunc,constructor->llvmFunc->getArg(0));


          //Generator::instance->m_vars.pop_back();
          builder->SetInsertPoint(entry);

          Constant* vtG = ConstantExpr::getGetElementPtr(vtable->getValueType(),vtable,(ArrayRef<Constant*>) {builder->getInt32(0),builder->getInt32(0),builder->getInt32(2)},true,1);
          builder->CreateStore(vtG,load);
          for(int i = 1;i < fullFuncs.size();i++) {
               Value* ptr2 = builder->CreateInBoundsGEP(builder->getInt8Ty(),load,ConstantInt::get(builder->getInt64Ty(), i * 8));
               Constant* vtGI = ConstantExpr::getGetElementPtr(vtable->getValueType(),vtable,(ArrayRef<Constant*>) {builder->getInt32(0),builder->getInt32(i),builder->getInt32(2)},true,1);
               builder->CreateStore(vtGI,ptr2);
          }

          //Generator::instance->m_vars.emplace_back();

          for(size_t i = 1;i < constructor->llvmFunc->arg_size();i++){
               constructor->llvmFunc->getArg(i)->setName(constructor->paramName[i - 1]);
               Generator::instance->createVar(constructor->llvmFunc->getArg(i),constructor->signage[i - 1],constructor->llvmFunc->getArg(i)->getType()->isPointerTy()?constructor->paramTypeName[i - 1]->mangle():"");
          }

          uint j = 0;
          for (size_t i = extending.has_value();i < vars.size();i++) {
               if(!vars[i]->_static) {
                    Value* val = builder->CreateStructGEP(strType,load,j + fullFuncs.size());
                    Value* gen = vars[i]->generate(builder);
                    builder->CreateStore(gen,val);
                    j++;
               }else vars[i]->generate(builder);
          }

          constructor->scope->generate(builder);
          Generator::instance->m_vars.pop_back();

          builder->CreateRetVoid();
          builder->SetInsertPoint((BasicBlock*) nullptr);
     }

     if(constructors.empty()) {
          FunctionType* ty = FunctionType::get(builder->getVoidTy(),{builder->getPtrTy()},false);
          Function* func = Function::Create(ty,GlobalValue::ExternalLinkage,Igel::Mangler::mangle(this,{},{},{},true),*Generator::instance->m_module);
          BasicBlock* entry = BasicBlock::Create(*Generator::m_contxt,"entry",func);
          builder->SetInsertPoint(entry);

          AllocaInst* alloc = builder->CreateAlloca(ptr);
          builder->CreateStore(func->getArg(0),alloc);
          LoadInst* load = builder->CreateLoad(alloc->getAllocatedType(),alloc);

          if(extending.has_value() && extending.value()->defaulfConstructor.has_value())builder->CreateCall(extending.value()->defaulfConstructor.value()->llvmFunc,func->getArg(0));
          if(extending.has_value() && !extending.value()->defaulfConstructor) {
               std::cerr << "No default constructor for " << extending.value()->mangle() << "manual call to super() needed in " << mangle() << std::endl;
          }

          builder->SetInsertPoint(entry);

          Constant* vtG = ConstantExpr::getGetElementPtr(vtable->getValueType(),vtable,(ArrayRef<Constant*>) {builder->getInt32(0),builder->getInt32(0),builder->getInt32(2)},true,1);
          builder->CreateStore(vtG,load);
          for(int i = 1;i < fullFuncs.size();i++) {
               Value* ptr2 = builder->CreateInBoundsGEP(builder->getInt8Ty(),load,ConstantInt::get(builder->getInt64Ty(), i * 8));
               Constant* vtGI = ConstantExpr::getGetElementPtr(vtable->getValueType(),vtable,(ArrayRef<Constant*>) {builder->getInt32(0),builder->getInt32(i),builder->getInt32(2)},true,1);
               builder->CreateStore(vtGI,ptr2);
          }
          builder->CreateRetVoid();
     }
}

std::string Class::mangle() {
     return Igel::Mangler::mangleTypeName(this);
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
     if(type == TokenType::id || type == TokenType::uninit)return PointerType::get(*Generator::m_contxt,0);
     if(type <= TokenType::_ulong) {
          int i = static_cast<int>(type) % 4;
          int b = (i + 1) * 8 + 8 * std::max(i - 1,0) + 16 * std::max(i - 2,0);
          return llvm::IntegerType::get(*Generator::m_contxt,b);
     }
     if(type == TokenType::_float)return llvm::IntegerType::getFloatTy(*Generator::m_contxt);
     if(type == TokenType::_double)return llvm::IntegerType::getDoubleTy(*Generator::m_contxt);
     if(type == TokenType::_bool)return llvm::IntegerType::getInt1Ty(*Generator::m_contxt);
     if(type == TokenType::_void)return Type::getVoidTy(*Generator::m_contxt);
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

llvm::Value* NodeTermFuncCall::generate(llvm::IRBuilder<>* builder) {
     Generator::classRet = nullptr;
     if(contained) {
          Generator::stump = true;
          contained.value()->generatePointer(builder);
          Generator::stump = false;
          if(Generator::classRet) {
               if(auto clz = dynamic_cast<NodeTermAcces*>(contained.value())) {
                    Class* clazz = Generator::classRet;
                    if(!clazz->funcIdMs.contains(name->name)) {
                         std::cerr << "Unkown function " << name->name;
                         exit(EXIT_FAILURE);
                    }
                    std::pair<uint,uint> fid = clazz->funcIdMs[name->name];
                    auto ptr = clz->generateClassPointer(builder);
                    //auto val = builder->CreateLoad(ptr->getType(),ptr);
                    auto ptrP = builder->CreateInBoundsGEP(builder->getPtrTy(),ptr,builder->getInt64(fid.first));
                    auto vtable = builder->CreateLoad(ptr->getType(),ptrP);


                    auto funcPtrGEP = builder->CreateInBoundsGEP(vtable->getType(),vtable,builder->getInt64(fid.second));
                    auto funcPtr = builder->CreateLoad(builder->getPtrTy(),funcPtrGEP);
                    std::vector<Value*> vals{ptr};
                    vals.reserve(exprs.size());
                    for (auto expr : exprs) vals.push_back(expr->generate(builder));
                    //clazz->getFuncsRec()[fid.first][fid.second]->llvmFunc->print(outs());
                    return builder->CreateCall(clazz->funcMap[fid.first][fid.second]->llvmFunc->getFunctionType(),funcPtr,vals);
                    //return builder->CreateCall(clazz->funcs[fid.first,fid.second]->llvmFunc->getFunctionType(),funcPtr,vals);
               }
          }else contained.value()->generate(builder);
     }
     std::vector<llvm::Value*> vals;
     vals.reserve(exprs.size());
     typeNames.clear();
     typeNames.reserve(exprs.size());
     for (const auto expr : exprs) {
          llvm::Value* val = expr->generate(builder);
          if(val->getType()->isPointerTy() || val->getType()->isIntegerTy(0))typeNames.push_back(Generator::typeNameRet);
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
     llvm::Value* val =  builder->CreateCall(callee.first,vals);
     return val;
}

/*llvm::Value* NodeTermStructNew::generate(llvm::IRBuilder<>* builder) {
     return Generator::instance->genStructVar(typeName->mangle());
}*/

llvm::Value* NodeTermClassNew::generate(llvm::IRBuilder<>* builder) {
     if(auto clazz = Generator::instance->m_file->findClass(typeName->mangle())){
          std::vector<Type*> types {builder->getPtrTy()};
          std::vector<BeContained*> names {new Name("")};
          std::vector<bool> sing = {true};

          std::vector<Value*> params {};
          params.reserve(exprs.size());
          if(paramType.empty() && !exprs.empty()) {
               for (auto expr : exprs) {
                    auto exprRet = expr->generate(builder);
                    params.push_back(exprRet);
                    Generator::typeNameRet = nullptr;
                    types.push_back(exprRet->getType());
                    names.push_back(Generator::typeNameRet);
                    sing.push_back(expr->_signed);
                    Generator::typeNameRet = nullptr;
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
          ArrayRef<Type*> llvmTypes {types};
          auto calle = Generator::instance->m_module->getOrInsertFunction(Igel::Mangler::mangle(typeName,types,names,sing,true),builder->getVoidTy(),llvmTypes);

          params.insert(params.begin(),ptr);
          //for (auto expr : exprs) params.push_back(expr->generate(builder));
          builder->CreateCall(calle,params);
          Generator::typeNameRet = typeName;
          return ptr;
     }
     std::cerr << "Unknown type " << typeName << std::endl;
     exit(EXIT_FAILURE);
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
          if(auto str = Generator::instance->m_file->findStruct(typeName.value()->mangle())) {

               sizeB = Generator::instance->m_module->getDataLayout().getTypeSizeInBits(str.value().first) / 8;
          }else if(auto clazz = Generator::instance->m_file->findClass(typeName.value()->mangle())) {
               sizeB = Generator::instance->m_module->getDataLayout().getTypeSizeInBits(clazz.value().first) / 8;
          }
     }else sizeB = Generator::instance->m_module->getDataLayout().getTypeSizeInBits(getType(static_cast<TokenType>(sid))) / 8;
     Value* arr = builder->CreateCall(_new,{ConstantInt::get(IntegerType::getInt32Ty(builder->getContext()),sizeB),
          ConstantInt::get(IntegerType::getInt32Ty(builder->getContext()),size.size() - 1),sizes});
     const llvm::FunctionCallee del = Generator::instance->m_module->getOrInsertFunction("free",builder->getVoidTy(),builder->getPtrTy());
     builder->CreateCall(del,sizes);
     return arr;
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