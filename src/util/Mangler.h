//
// Created by igel on 09.02.24.
//

#ifndef MANGLER_H
#define MANGLER_H
#include <string>

#include "../types.h"

namespace IgCmp {
    struct Mangled {
        bool mangled;
        BeContained* cont;
        std::string name;
    };

    std::string mangle(BeContained* cont);
    std::string mangleImpl(BeContained* cont);
    std::string mangle(std::vector<llvm::Type*> types,std::vector<BeContained*> typeNames,std::vector<bool> signage);
    std::string mangle(BeContained* cont,std::vector<llvm::Type*> types,std::vector<BeContained*> typeNames,std::vector<bool> signage);
}

#endif //MANGLER_H
