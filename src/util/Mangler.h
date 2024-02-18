//
// Created by igel on 09.02.24.
//

#ifndef MANGLER_H
#define MANGLER_H
#include <string>

#include "../types.h"

namespace Igel {
    class Mangler{
        struct Mangled {
            bool mangled;
            BeContained* cont;
            std::string name;
        };
    public:
        static std::string mangleTypeName(BeContained* cont);
        static std::string mangleName(BeContained* cont);
        static std::string mangle(BeContained* cont);
        static std::string mangle(std::string name);
        //static std::string mangleImpl(BeContained* cont);
        static std::string mangle(std::vector<llvm::Type*> types,std::vector<BeContained*> typeNames,std::vector<bool> signage);
        static std::string mangle(BeContained* cont,std::vector<llvm::Type*> types,std::vector<BeContained*> typeNames,std::vector<bool> signage);
    };
}
#endif //MANGLER_H
