//
// Created by igel on 09.02.24.
//

#include "Mangler.h"

#include <utility>

std::string IgCmp::mangle(BeContained* cont) {
    std::string str = "_Z";
    str += mangleImpl(cont);
    str += "E";
    return str;
}

std::string IgCmp::mangleImpl(BeContained* cont) {
    std::string str;
    if(cont->contType.has_value()) {
        std::vector<BeContained*> conts;
        BeContained* cpy = cont;
        conts.push_back(cpy);
        while (cpy->contType.has_value()) {
            conts.push_back(cpy->contType.value());
            cpy = cpy->contType.value();
        }
        for(uint i = conts.size() - 1;i > 0;i--) {
            //TODO remember to check for Pointer (weather adding 'P')
            str += 'P';
            str += std::to_string(conts[i]->name.size());
            str += conts[i]->name;
        }
    }else {
        //TODO remember to check for Pointer (weather adding 'P')
        str += 'P';
        str += std::to_string(cont->name.size());
        str += cont->name;
    }

    return str;
}

std::string IgCmp::mangle(std::vector<llvm::Type*> types, std::vector<BeContained*> typeNames,std::vector<bool> signage) {
    std::string str;
    for(uint i = 0;i < types.size();i++) {
        std::string name;
        char c = -2;
        if(types[i]->isIntegerTy(8))c = 'c';
        else if(types[i]->isIntegerTy(16))c = 's';
        else if(types[i]->isIntegerTy(32))c = 'i';
        else if(types[i]->isIntegerTy(64))c = 'l';
        else if(types[i]->isDoubleTy())c = 'd';
        else if(types[i]->isFloatTy())c = 'f';
        else name = mangleImpl(typeNames[i]);
        c += signage[i];
        if(c >= 0)str += c;
        else str += name;
    }
    if(types.empty())str += "v";
    return str;
}

std::string IgCmp::mangle(BeContained* cont, std::vector<llvm::Type*> types, std::vector<BeContained*> typeNames,std::vector<bool> signage) {
    std::string str = "_Z";
    str += mangleImpl(cont);
    str += "E";
    str += mangle(std::move(types),std::move(typeNames),std::move(signage));
    return str;
}
