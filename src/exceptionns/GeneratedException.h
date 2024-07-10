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
    };
}

#endif //GENERATEDEXCEPTION_H
