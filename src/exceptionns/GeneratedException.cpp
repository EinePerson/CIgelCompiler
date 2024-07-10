//
// Created by igel on 03.07.24.
//

#include "GeneratedException.h"
#include "./../../srcLib/Exception.h"

#include <llvm/IR/DerivedTypes.h>
#include <llvm/IR/IRBuilder.h>

#include "../types.h"
#include "../langMain/codeGen/Generator.h"

void Igel::GeneratedException::throwException(llvm::IRBuilder<> *builder, std::string name,std::string message) {
    GlobalVariable* exName = builder->CreateGlobalString(name);
    GlobalVariable* exMsg = builder->CreateGlobalString(message);

    llvm::FunctionCallee constr = Generator::instance->m_module->getOrInsertFunction("_ZN6StringC1EPKc",FunctionType::get(builder->getVoidTy(),{builder->getPtrTy(),builder->getPtrTy()},false));
    llvm::FunctionCallee constrExc = Generator::instance->m_module->getOrInsertFunction("_ZN9ExceptionC1E6StringS0_",FunctionType::get(builder->getVoidTy(),
        {builder->getPtrTy(),builder->getPtrTy(),builder->getPtrTy()},false));
    llvm::FunctionCallee throwInt = Generator::instance->m_module->getOrInsertFunction("_ZN9Exception13throwInternalEv",FunctionType::get(builder->getVoidTy(),{builder->getPtrTy()},false));

    StructType* str = StructType::create(builder->getContext(),"class.String");
    str->setBody({builder->getPtrTy(),builder->getInt32Ty(),ArrayType::get(builder->getInt8Ty(),4)});
    StructType* excpStr = StructType::create(builder->getContext(),"class.Exception");
    excpStr->setBody(builder->getPtrTy(),str,str,str);

    AllocaInst* nameAlloc = builder->CreateAlloca(str);
    AllocaInst* msgAlloc = builder->CreateAlloca(str);
    AllocaInst* excpAlloc = builder->CreateAlloca(excpStr);

    builder->CreateCall(constr,{nameAlloc,exName});
    builder->CreateCall(constr,{msgAlloc,exMsg});

    builder->CreateCall(constrExc,{excpAlloc,nameAlloc,msgAlloc});
    builder->CreateCall(throwInt,{excpAlloc});
}
