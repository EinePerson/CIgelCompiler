//
// Created by igel on 19.11.23.
//

#include <llvm/IR/Verifier.h>

#include "Generator.h"

llvm::Value* NodeTermId::generate(llvm::IRBuilder<>* builder) {
     if(contained) {
          return contained.value()->generate(builder);
     }
     AllocaInst* ptr = (AllocaInst*) (Generator::instance->getVar(id.value.value())->alloc);
     return builder->CreateLoad(ptr->getAllocatedType(),ptr);
}

llvm::Value* NodeTermId::generatePointer(llvm::IRBuilder<>* builder) {
     if(contained) {
          return contained.value()->generatePointer(builder);
     }
     Value* ptr = Generator::instance->getVar(id.value.value())->alloc;
     return ptr;
}

llvm::Value* NodeTermArrayAcces::generate(llvm::IRBuilder<>* builder) {
     if(contained) {
          contained.value()->generate(builder);
     }
     ArrayVar* var = static_cast<ArrayVar*>(Generator::instance->getVar(id.value.value()));

     Value* ep = var->alloc;
     Value* _if = nullptr;
     for (size_t i = 0;i < exprs.size();i++) {
          Value* size = exprs[i]->generate(builder);
          Value* arrSize = builder->CreateLoad(IntegerType::getInt32Ty(builder->getContext()),var->sizes[i]);
          if(i == 0) {
               _if = builder->CreateCmp(CmpInst::ICMP_UGE,size,arrSize);
          }else {
               _if = builder->CreateOr(_if,builder->CreateCmp(CmpInst::ICMP_UGE,size,arrSize));
          }
          ep = builder->CreateGEP(var->types[var->types.size() - exprs.size() - 1],ep,size);
     }
     llvm::Function* parFunc = builder->GetInsertBlock()->getParent();
     llvm::BasicBlock* then = llvm::BasicBlock::Create(builder->getContext(),"then",parFunc);
     llvm::BasicBlock* elseB = llvm::BasicBlock::Create(builder->getContext(),"else");
     llvm::BasicBlock* merge = llvm::BasicBlock::Create(builder->getContext(),"ifcont");

     builder->CreateCondBr(_if,then,elseB);

     builder->SetInsertPoint(then);
     //llvm::Value* thenVal = generateS(scope,builder);
     //TODO add exception throwing when IndexOutOfBounds

     builder->CreateBr(merge);
     then = builder->GetInsertBlock();

     parFunc->insert(parFunc->end(),elseB);
     builder->SetInsertPoint(elseB);
     builder->CreateBr(merge);
     elseB = builder->GetInsertBlock();

     parFunc->insert(parFunc->end(),merge);
     builder->SetInsertPoint(merge);
     return builder->CreateLoad(var->types[var->types.size() - exprs.size() - 1],ep);
}

llvm::Value* NodeTermArrayAcces::generatePointer(llvm::IRBuilder<>* builder) {
     if(contained) {
          contained.value()->generate(builder);
     }
     ArrayVar* var = static_cast<ArrayVar*>(Generator::instance->getVar(id.value.value()));

     Value* ep = var->alloc;
     Value* _if = nullptr;
     for (size_t i = 0;i < exprs.size();i++) {
          Value* size = exprs[i]->generate(builder);
          Value* arrSize = builder->CreateLoad(IntegerType::getInt32Ty(builder->getContext()),var->sizes[i]);
          if(i == 0) {
               _if = builder->CreateCmp(CmpInst::ICMP_UGE,size,arrSize);
          }else {
               _if = builder->CreateOr(_if,builder->CreateCmp(CmpInst::ICMP_UGE,size,arrSize));
          }
          ep = builder->CreateGEP(var->types[var->types.size() - exprs.size() - 1],ep,size);
     }
     llvm::Function* parFunc = builder->GetInsertBlock()->getParent();
     llvm::BasicBlock* then = llvm::BasicBlock::Create(builder->getContext(),"then",parFunc);
     llvm::BasicBlock* elseB = llvm::BasicBlock::Create(builder->getContext(),"else");
     llvm::BasicBlock* merge = llvm::BasicBlock::Create(builder->getContext(),"ifcont");

     builder->CreateCondBr(_if,then,elseB);

     builder->SetInsertPoint(then);
     //llvm::Value* thenVal = generateS(scope,builder);
     //TODO add exception throwing when IndexOutOfBounds

     builder->CreateBr(merge);
     then = builder->GetInsertBlock();

     parFunc->insert(parFunc->end(),elseB);
     builder->SetInsertPoint(elseB);
     builder->CreateBr(merge);
     elseB = builder->GetInsertBlock();

     parFunc->insert(parFunc->end(),merge);
     builder->SetInsertPoint(merge);
     return ep;
}

llvm::Value* NodeTermStructAcces::generate(llvm::IRBuilder<>* builder) {
     StructVar* var = dynamic_cast<StructVar*>(Generator::instance->getVar(id.value.value()));
     uint fid = var->vars[acc.value.value()];
     /*Value* val = nullptr;
     if(contained) {
          val = contained.value()->generate(builder);
          //val = builder->CreateStructGEP(gen->getType(),gen,
     }else {
          val = builder->CreateLoad(var->type,var->alloc);
     }
     //var->strType->print(outs());
     Value* ptr = builder->CreateStructGEP(var->strType,val,fid);+*/
     //for (auto type : var->types)type->print(outs());
     return builder->CreateLoad(var->types[fid],this->generatePointer(builder));
}

llvm::Value* NodeTermStructAcces::generatePointer(llvm::IRBuilder<>* builder) {
     Value* val;
     StructVar* var = static_cast<StructVar*>(Generator::instance->getVar(id.value.value()));
     if(contained) {
          val = contained.value()->generate(builder);
          //val = builder->CreateStructGEP(gen->getType(),gen,
     }else {
          val = builder->CreateLoad(var->type,var->alloc);
     }
     uint fid = var->vars[acc.value.value()];
     //var->strType->print(outs());
     Value* ptr = builder->CreateStructGEP(var->strType,val,fid);
     return ptr;
}

llvm::Value* NodeStmtPirimitiv::generate(llvm::IRBuilder<>* builder) {
     Value* val = expr.has_value()?expr.value()->generate(builder) : ConstantInt::get(Generator::getType(static_cast<TokenType>(sid)),0);
     Generator::instance->createVar(id.value.value(),Generator::getType(static_cast<TokenType>(sid)),val);
     return val;
}

llvm::Value* NodeStmtStructNew::generate(llvm::IRBuilder<>* builder) {
     if(auto structT = Generator::instance->m_file->findStruct(typeName)) {
          auto type = PointerType::get(builder->getContext(),0);
          AllocaInst* alloc = builder->CreateAlloca(type);
          Value* gen = term->generate(builder);
          builder->CreateStore(gen,alloc);

          auto var = new StructVar(alloc);
          var->type = type;
          var->strType = structT.value().first;
          var->types = structT.value().second->types;
          for(size_t i = 0;i < structT.value().second->vars.size();i++) {
               NodeStmtLet* let = structT.value().second->vars[i];
               auto t = let->id;
               std::string str = structT.value().second->vars[i]->id.value.value();
               var->vars[str] = i;
          }
          Generator::instance->m_vars.back()[id.value.value()] = var;
          return gen;
     }else {
          std::cerr << "Undeclarde Type " << typeName << std::endl;
          exit(EXIT_FAILURE);
     }

     //Generator::instance->genStructVar(typeName,id);
     //Generator::instance->genStructVar(typeName,id);
     /*if(auto structT = Generator::instance->m_file->findStruct(typeName)) {
          auto type = PointerType::get(structT,0);
          StructVar* var = new StructVar;
          var->alloc = builder->CreateAlloca(type);
          Generator::currentVar.push_back(var);



          Generator::currentVar.pop_back();
     }*/
}

llvm::Value* NodeStmtArr::generate(llvm::IRBuilder<>* builder) {
     Type* type = getType((TokenType) sid);
     auto* var = new ArrayVar(builder->CreateAlloca(type));
     var->types.push_back(type);
     for (auto i : size) {
          type = ArrayType::get(type,i);
          var->types.push_back(type);
          AllocaInst* alloc = builder->CreateAlloca(IntegerType::getInt32Ty(builder->getContext()));
          builder->CreateStore(ConstantInt::get(IntegerType::getInt32Ty(builder->getContext()),i),alloc);
          var->sizes.push_back(alloc);
     }
     Generator::instance->m_vars.back().insert_or_assign(id.value.value(),var);
     return builder->CreateStore(ConstantInt::get(type,0),var->alloc);
}

llvm::Value* NodeStmtExit::generate(llvm::IRBuilder<>* builder) {
     Value* val = expr->generate(builder);
     Generator* gen = Generator::instance;

     //Generator::instance->m_module->print(outs(),nullptr);
     //if(val->getType()->isIntegerTy())
          return gen->exitG(val);
     //else return  gen->exitG(builder->CreateLoad(IntegerType::getInt32Ty(builder->getContext()),val));
}

llvm::Value* NodeStmtIf::generate(llvm::IRBuilder<>* builder){
     llvm::Value* val = expr[0]->generate(builder);
     for (size_t i = 1;i < expr.size();i++) {
          if(exprCond[i - 1] == TokenType::_and)val = builder->CreateLogicalAnd(val,expr[i]->generate(builder));
          else val = builder->CreateLogicalOr(val,expr[i]->generate(builder));
     }

     llvm::Function* parFunc = builder->GetInsertBlock()->getParent();
     llvm::BasicBlock* then = llvm::BasicBlock::Create(builder->getContext(),"then",parFunc);
     llvm::BasicBlock* elseB = llvm::BasicBlock::Create(builder->getContext(),"else");
     llvm::BasicBlock* merge = llvm::BasicBlock::Create(builder->getContext(),"ifcont");

     builder->CreateCondBr(val,then,elseB);

     builder->SetInsertPoint(then);
     llvm::Value* thenVal = generateS(scope,builder);

     builder->CreateBr(merge);
     then = builder->GetInsertBlock();

     llvm::Value* elseV = nullptr;
     if(scope_else) {

          parFunc->insert(parFunc->end(),elseB);
          builder->SetInsertPoint(elseB);
          elseV = generateS(scope_else.value(),builder);
          builder->CreateBr(merge);
          elseB = builder->GetInsertBlock();
     }else if(else_if) {
          parFunc->insert(parFunc->end(),elseB);
          builder->SetInsertPoint(elseB);
          elseV = else_if.value()->generate(builder);
          builder->CreateBr(merge);
          elseB = builder->GetInsertBlock();
     }else {
          parFunc->insert(parFunc->end(),elseB);
          builder->SetInsertPoint(elseB);
          builder->CreateBr(merge);
          elseB = builder->GetInsertBlock();
     }
     parFunc->insert(parFunc->end(),merge);
     builder->SetInsertPoint(merge);
     /*llvm::PHINode* phi = builder->CreatePHI(thenVal->getType(),2,"iftmp");
     phi->addIncoming(thenVal,then);
     if(elseV) {
          phi->addIncoming(elseV,elseB);
     }*/
     //Generator::instance->m_module->print(outs(),nullptr);
     return elseB != nullptr?elseB:then;
}

llvm::Value* NodeStmtReassign::generate(llvm::IRBuilder<>* builder) {
     //Var* var = Generator::instance->getVar(id.value.value());
     //TODO REMEMBER
     Value* val = id->generatePointer(builder);
     //Value* val = id->generate(builder);
     if(op == TokenType::eq) {
          //Generator::instance->m_module->print(outs(),nullptr);
          return builder->CreateStore(expr->generate(builder),val);
     }else {
          Value* load = builder->CreateLoad(val->getType(),val);
          Value* _new = expr->generate(builder);
          load = builder->CreatePtrToInt(load,_new->getType());
          Value* res = generateReassing(load,_new,op,builder);
          return builder->CreateStore(res,val);
     }
}

llvm::Value* NodeStmtArrReassign::generate(llvm::IRBuilder<>* builder) {
     /*ArrayVar* var = static_cast<ArrayVar *>(Generator::instance->getVar(id.value.value()));

     Value* ep = var->alloc;
     Value* _if = nullptr;
     /*for (auto expr : exprs) {

          ep = builder->CreateGEP(var->types[var->types.size() - exprs.size() - 1],ep,expr->generate(builder));
     }*//*
     for (size_t i = 0;i < exprs.size();i++) {
          Value* size = exprs[i]->generate(builder);
          Value* arrSize = builder->CreateLoad(IntegerType::getInt32Ty(builder->getContext()),var->sizes[i]);
          if(i == 0) {
               _if = builder->CreateCmp(CmpInst::ICMP_UGE,size,arrSize);
          }else {
               _if = builder->CreateOr(_if,builder->CreateCmp(CmpInst::ICMP_UGE,size,arrSize));
          }
          ep = builder->CreateGEP(var->types[var->types.size() - exprs.size() - 1],ep,size);
     }
     llvm::Function* parFunc = builder->GetInsertBlock()->getParent();
     llvm::BasicBlock* then = llvm::BasicBlock::Create(builder->getContext(),"then",parFunc);
     llvm::BasicBlock* elseB = llvm::BasicBlock::Create(builder->getContext(),"else");
     llvm::BasicBlock* merge = llvm::BasicBlock::Create(builder->getContext(),"ifcont");

     builder->CreateCondBr(_if,then,elseB);

     builder->SetInsertPoint(then);
     //llvm::Value* thenVal = generateS(scope,builder);
     //TODO add exception throwing when IndexOutOfBounds

     builder->CreateBr(merge);
     then = builder->GetInsertBlock();

     parFunc->insert(parFunc->end(),elseB);
     builder->SetInsertPoint(elseB);
     builder->CreateBr(merge);
     elseB = builder->GetInsertBlock();

     parFunc->insert(parFunc->end(),merge);
     builder->SetInsertPoint(merge);*/

     Value* ep = acces->generatePointer(builder);
     if(op == TokenType::eq) {
          Value* gen = expr.value()->generate(builder);
          //Generator::instance->m_module->print(outs(),nullptr);
          Value* val = builder->CreateStore(gen,ep);
          return val;
     }else {
          ArrayVar* var = static_cast<ArrayVar*>(Generator::instance->getVar(acces->id.value.value()));
          Value* load = builder->CreateLoad(var->types[var->types.size() - acces->exprs.size() - 1],ep);
          Value* _new = expr.value()->generate(builder);
          Value* res = generateReassing(load,_new,op,builder);
          return builder->CreateStore(res,ep);
     }
};

llvm::Value* Return::generate(llvm::IRBuilder<>* builder)  {
     if(val) {
          return  builder->CreateRet(val.value()->generate(builder));
     }
     return  builder->CreateRetVoid();
};

int Types::getSize(TokenType type) {
     if(type > TokenType::_ulong)return -1;
     const int i = static_cast<int>(type) % 4;
     return  (i + 1) * 8 + 8 * std::max(i - 1,0) + 16 * std::max(i - 2,0);
}

void Struct::generateSig(llvm::IRBuilder<>* builder) {
     for(uint i = 0; i < this->types.size();i++) {
          if(types[i] == nullptr) {
               types[i] = PointerType::get(builder->getContext(),0);
          }
          //types[i]->print(outs());
     }
     Generator::instance->m_file->structs[name] = std::make_pair(StructType::create(builder->getContext(),name),this);
}

void Struct::generate(llvm::IRBuilder<>* builder) {
     StructType* type = Generator::instance->m_file->findStruct(name).value().first;
     std::vector<Type*> types;
     for (auto var : vars) {
          Type* t = getType(var->type);
          types.push_back(t);
     }
     type->setBody(types,false);

}

llvm::FunctionCallee getFunction(std::string name,std::vector<Type*> params) {
     std::optional<FunctionCallee> callee = Generator::instance->m_file->findFunc(name,params);
     if(!callee.has_value()) {
          std::cerr << "Could not find function " << name << "\n";
          exit(EXIT_FAILURE);
     }
     return callee.value();
     /*FunctionType* type = FunctionType::get(Type::getVoidTy(Generator::instance->m_module->getContext()), params, false);
     return  Generator::instance->m_module->getOrInsertFunction(func->name,type);*/
}

llvm::Type* getType(TokenType type) {
     if(type <= TokenType::_ulong) {
          int i = static_cast<int>(type) % 4;
          int b = (i + 1) * 8 + 8 * std::max(i - 1,0) + 16 * std::max(i - 2,0);
          return llvm::IntegerType::get(*Generator::m_contxt,b);
     }
     if(type == TokenType::_void)return Type::getVoidTy(Generator::instance->m_module->getContext());
     return nullptr;
}

llvm::Value* generateReassing(llvm::Value* load,llvm::Value* _new,TokenType op,llvm::IRBuilder<>* builder) {
     switch (op) {
          case TokenType::inc:
               return  builder->CreateAdd(load,ConstantInt::get(IntegerType::getInt8Ty(Generator::instance->m_module->getContext()),1));
          case TokenType::dec:
               return builder->CreateSub(load,ConstantInt::get(IntegerType::getInt8Ty(Generator::instance->m_module->getContext()),1));
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
     return  llvm::ConstantInt::get(type,std::stoi(int_lit.value.value()),sid <= 3);
}

llvm::Value* NodeTermNew::generate(llvm::IRBuilder<>* builder) {
     return Generator::instance->genStructVar(typeName);
}

llvm::Type* getType(llvm::Type* type) {
     return type == nullptr?PointerType::get(Generator::instance->m_module->getContext(),0):type;
}