//
// Created by igel on 03.07.24.
//

#ifndef GENERATEDEXCEPTION_H
#define GENERATEDEXCEPTION_H
#include <string>
#include <llvm/IR/IRBuilder.h>

namespace Igel {

    class GeneratedException {
    public:
        static void throwException(llvm::IRBuilder<> *builder, std::string name,std::string message);
        static void exception(llvm::IRBuilder<> *builder, std::string msg,std::vector<llvm::Value*>);
    };
}

#endif //GENERATEDEXCEPTION_H
