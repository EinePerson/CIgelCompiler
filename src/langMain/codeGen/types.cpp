//
// Created by igel on 19.11.23.
//

#include <llvm/IR/Verifier.h>

#include "Generator.h"
#include "../../util/Mangler.h"

llvm::Value* NodeTermId::generate(llvm::IRBuilder<>* builder) {
     if(contained) {
          return contained.value()->generate(builder);
     }
     Var* var = Generator::instance->getVar(Igel::Mangler::mangleName(cont));
     if(auto str = dynamic_cast<StructVar*>(var))Generator::typeNameRet = str->str;
     _signed = var->_signed;
     auto* ptr = (AllocaInst*) (var->alloc);
     return builder->CreateLoad(ptr->getAllocatedType(),ptr);
}

llvm::Value* NodeTermId::generatePointer(llvm::IRBuilder<>* builder) {
     if(contained) {
          return contained.value()->generatePointer(builder);
     }
     Var* var = Generator::instance->getVar(Igel::Mangler::mangleName(cont));
     if(auto str = dynamic_cast<StructVar*>(var))Generator::typeNameRet = str->str;
     //if(auto arr = dynamic_cast<ArrayVar*>(var))Generator::typeNameRet.
     Value* ptr = var->alloc;
     return ptr;
}

llvm::Value* NodeTermArrayAcces::generate(llvm::IRBuilder<>* builder) {
     if(contained) {
          if(dynamic_cast<NodeTermStructAcces*>(contained.value())) {
               Struct* str = Generator::structRet;
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
     return builder->CreateLoad(var->type,generatePointer(builder));
}

llvm::Value* NodeTermArrayAcces::generatePointer(llvm::IRBuilder<>* builder) {
     Value* ptr = nullptr;
     ArrayVar* var = nullptr;
     if(contained) {
          ptr = contained.value()->generatePointer(builder);
     }else {
          var = static_cast<ArrayVar*>(Generator::instance->getVar(Igel::Mangler::mangleName(cont)));
          _signed = var->_signed;
          if(var->typeName.has_value()) {
               auto strRet = Generator::instance->m_file->findStruct(var->typeName.value());
               Generator::structRet = strRet.value().second;
          }
     }
     if(!ptr && !var) {
          std::cerr << "Neither contained value nor variable" << std::endl;
          exit(EXIT_FAILURE);
     }
     uint i = 0;
     while (i < exprs.size()) {
          ptr = builder->CreateStructGEP(Generator::arrTy,ptr?ptr:var->alloc,1);
          ptr = builder->CreateLoad(PointerType::get(builder->getContext(),0),ptr);
          ptr = builder->CreateInBoundsGEP(Generator::arrTy,ptr,exprs[i]->generate(builder));
          i++;
     }
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
          val = contained.value()->generatePointer(builder);
          str = Generator::structRet;
     }else {
          var = dynamic_cast<StructVar*>(Generator::instance->getVar(id->mangle()));
          val = builder->CreateLoad(var->type,var->alloc);
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
          return builder->CreateLoad(var->types[fid],ptr);
     }
     if(const auto strT = Generator::instance->m_file->findStruct(str->typeName[fid]->mangle())) {
          Generator::structRet = strT.value().second;
     }else Generator::structRet = nullptr;
     return builder->CreateLoad(str->types[fid],ptr);
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
     }else {
          var = dynamic_cast<StructVar*>(Generator::instance->getVar(id->mangle()));
          val = builder->CreateLoad(var->type,var->alloc);
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
          return ptr;
     }
     if(const auto strT = Generator::instance->m_file->findStruct(str->typeName[fid]->mangle())) {
          Generator::structRet = strT.value().second;
     }else Generator::structRet = nullptr;
     return ptr;
}

llvm::Value* NodeStmtPirimitiv::generate(llvm::IRBuilder<>* builder) {
     Value* val = expr.has_value()?expr.value()->generate(builder) : ConstantInt::get(Generator::getType(static_cast<TokenType>(sid)),0);
     Generator::instance->createVar(mangle(),Generator::getType(static_cast<TokenType>(sid)),val,sid <= 3);
     return val;
}

std::string NodeStmtPirimitiv::mangle() {
     return "_Z" + Igel::Mangler::mangle(this);
}

std::string NodeStmtNew::mangle() {
     return Igel::Mangler::mangle(this);
}

std::string NodeStmtArr::mangle() {
     return Igel::Mangler::mangleName(this);
}

llvm::Value* NodeStmtStructNew::generate(llvm::IRBuilder<>* builder) {
     if(auto structT = Generator::instance->m_file->findStruct(typeName->mangle())) {
          auto type = PointerType::get(builder->getContext(),0);
          AllocaInst* alloc = builder->CreateAlloca(type);
          Value* gen = term->generate(builder);
          builder->CreateStore(gen,alloc);

          auto var = new StructVar(alloc);
          var->type = type;
          var->strType = structT.value().first;
          var->str = structT.value().second;
          var->types = structT.value().second->types;
          for(size_t i = 0;i < structT.value().second->vars.size();i++) {
               NodeStmtLet* let = structT.value().second->vars[i];
               std::string str = structT.value().second->vars[i]->mangle();
               if(auto pirim = dynamic_cast<NodeStmtPirimitiv*>(let))var->signage.push_back(pirim->sid <= (char) TokenType::_long);
               else var->signage.push_back(false);
               //var->signage.push_back(let-> <= TokenType::_long);
               var->vars[str] = i;
          }
          if(structT.value().second->varIdMs.empty())structT.value().second->varIdMs = var->vars;
          if(!structT.value().second->strType)structT.value().second->strType = var->strType;
          //Generator::instance->m_vars.back()[Igel::Mangler::mangle(name)] = var;
          Generator::instance->createVar(Igel::Mangler::mangle(name),var);
          return gen;
     }else {
          std::cerr << "Undeclarde Type " << typeName->mangle() << std::endl;
          exit(EXIT_FAILURE);
     }
}

Value* createArr(std::vector<Value*> sizes,std::vector<Type*> types,uint i,llvm::IRBuilder<>* builder) {
     if(i == 0)return ConstantInt::get(types[i],0);
     const FunctionCallee _new = Generator::instance->m_module->getOrInsertFunction("_Znam",PointerType::get(builder->getContext(),0),IntegerType::getInt64Ty(builder->getContext()));
     Value* var = builder->CreateCall(_new,builder->CreateMul(sizes[i - 1],ConstantInt::get(IntegerType::getInt64Ty(builder->getContext()),12)));

     return var;
}

llvm::Value* NodeStmtArr::generate(llvm::IRBuilder<>* builder) {
     Value* val = llvm::ConstantPointerNull::get(llvm::PointerType::get(builder->getContext(),0));
     if(term)val = term->generate(builder);
     auto* var = new ArrayVar(builder->CreateAlloca(Generator::arrTy),sid <= 3,typeName != nullptr?Igel::Mangler::mangleTypeName(typeName):(std::optional<std::string>) {});
     var->size = size;
     var->type = getType(static_cast<TokenType>(sid));
     builder->CreateStore(val,var->alloc);
     //Generator::instance->m_vars.back().insert_or_assign(mangle(),var);
     Generator::instance->createVar(mangle(),var);
     return val;
}

llvm::Value* NodeStmtExit::generate(llvm::IRBuilder<>* builder) {
     Value* val = expr->generate(builder);
     Generator* gen = Generator::instance;

     return gen->exitG(val);
}

llvm::Value* NodeStmtScope::generate(llvm::IRBuilder<>* builder) {
     llvm::Value* val = nullptr;
     Generator::lastUnreachable = false;
     for (const auto stmt : stmts) {
          val = stmt->generate(builder);
     }
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
          Value* load = builder->CreateLoad(val->getType(),val);
          Value* _new = nullptr;
          if(expr != nullptr)_new = expr->generate(builder);
          load = builder->CreateLoad(static_cast<AllocaInst*>(val)->getAllocatedType(),val);
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

std::string IgFunction::mangle() {
     return Igel::Mangler::mangle(this,paramType,paramTypeName,signage);
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
                    types[i] = Generator::arrTy;
                    break;
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

void Struct::generate(llvm::IRBuilder<>* builder) {
     StructType* type = Generator::instance->m_file->findStruct(mangle()).value().first;
     for (auto var : vars) {
          //TODO maybe mangle
          varIds.push_back(var->name);
     }
     type->setBody(types,false);
}

std::string Struct::mangle() {
     return Igel::Mangler::mangleTypeName(this);
}

void Class::generateSig(llvm::IRBuilder<>* builder) {

}

void Class::generate(llvm::IRBuilder<>* builder) {

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
     if(type <= TokenType::_ulong) {
          int i = static_cast<int>(type) % 4;
          int b = (i + 1) * 8 + 8 * std::max(i - 1,0) + 16 * std::max(i - 2,0);
          return llvm::IntegerType::get(*Generator::m_contxt,b);
     }
     if(type == TokenType::_float)return llvm::IntegerType::getFloatTy(*Generator::m_contxt);
     if(type == TokenType::_double)return llvm::IntegerType::getDoubleTy(*Generator::m_contxt);
     if(type == TokenType::_bool)return llvm::IntegerType::getInt1Ty(*Generator::m_contxt);
     if(type == TokenType::_void)return Type::getVoidTy(*Generator::m_contxt);
     if(type == TokenType::id || type == TokenType::uninit)return PointerType::get(*Generator::m_contxt,0);
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
     if(contained)contained.value()->generate(builder);
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

llvm::Value* NodeTermNew::generate(llvm::IRBuilder<>* builder) {
     return Generator::instance->genStructVar(typeName);
}

llvm::Value* NodeTermArrNew::generate(llvm::IRBuilder<>* builder) {
     const FunctionCallee _sizNes = Generator::instance->m_module->getOrInsertFunction("_Znam",PointerType::get(builder->getContext(),0),IntegerType::getInt64Ty(builder->getContext()));
     Value* sizes = builder->CreateCall(_sizNes,ConstantInt::get(IntegerType::getInt64Ty(builder->getContext()),4 * size.size()));
     Value* ptr = builder->CreateInBoundsGEP(IntegerType::getInt32Ty(builder->getContext()),sizes,ConstantInt::get(IntegerType::getInt64Ty(builder->getContext()),0));
     builder->CreateStore(size[0]->generate(builder),ptr);
     for (uint i = 1; i < size.size();i++) {
          ptr = builder->CreateInBoundsGEP(IntegerType::getInt32Ty(builder->getContext()),sizes,ConstantInt::get(IntegerType::getInt64Ty(builder->getContext()),i));
          builder->CreateStore(size[i]->generate(builder),ptr);
     }
     const FunctionCallee _new = Generator::instance->m_module->getOrInsertFunction("createArray",Generator::arrTy,
          IntegerType::getInt8Ty(builder->getContext()),IntegerType::getInt32Ty(builder->getContext()),PointerType::get(builder->getContext(),0));
     Value* arr = builder->CreateCall(_new,{ConstantInt::get(IntegerType::getInt8Ty(builder->getContext()),sid),ConstantInt::get(IntegerType::getInt32Ty(builder->getContext()),size.size() - 1),sizes});
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