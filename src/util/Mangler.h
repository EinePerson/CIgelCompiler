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
        ///\brief member indicates weather the first parameter is the object the function is called on
        static std::string mangle(BeContained* cont,bool member = false,bool constructor = false,bool destructor = false);
        static std::string mangle(std::string name);
        ///\brief member indicates weather the first parameter is the object the function is called on
        static std::string mangle(std::vector<llvm::Type*> types,std::vector<BeContained*> typeNames,std::vector<bool> signage,bool member = false,bool constructor = false);
        ///\brief member indicates weather the first parameter is the object the function is called on
        static std::string mangle(BeContained* cont,std::vector<llvm::Type*> types,std::vector<BeContained*> typeNames,std::vector<bool> signage,bool member = false,bool constructor = false,bool destructor = false);
        static char typeToChar(llvm::Type *type, bool _signed);
    };
}
#endif //MANGLER_H
