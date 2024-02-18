//
// Created by igel on 09.02.24.
//

#include "Mangler.h"

#include <string>
#include <utility>

/*std::string IgCmp::mangle(BeContained* cont) {
    std::string str = "_Z";
    str += mangleImpl(cont);
    return str;
}*/

std::string Igel::Mangler::mangleTypeName(BeContained *cont) {
    std::string str;
    std::vector<BeContained*> conts;
    BeContained* cpy = cont;
    conts.push_back(cpy);
    while (cpy->contType.has_value() && cpy->contType.value() != nullptr) {
        cpy = cpy->contType.value();
        conts.push_back(cpy->contType.value());
    }

    str += conts[conts.size() - 1]->name;
    for(int i = conts.size() - 2;i >= 0;i--) {
        str += "::";
        str += conts[i]->name;
    }

    return str;
}

std::string Igel::Mangler::mangleName(BeContained *cont) {
    return "_Z" + mangle(cont);
}

std::string Igel::Mangler::mangle(BeContained* cont) {
    std::string str;
    if(cont->contType.has_value() && cont->contType.value() != nullptr) {
        std::vector<BeContained*> conts;
        BeContained* cpy = cont;
        conts.push_back(cpy);
        while (cpy->contType.has_value() && cpy->contType.value() != nullptr) {
            conts.push_back(cpy->contType.value());
            cpy = cpy->contType.value();
        }

        str += "N";
        for(int i = conts.size() - 1;i >= 0;i--) {
            str += std::to_string(conts[i]->name.size());
            str += conts[i]->name;
        }
        str += "E";
    }else {
        str += std::to_string(cont->name.size());
        str += cont->name;
    }

    return str;
}

std::string Igel::Mangler::mangle(std::string name) {
    return "_Z" + std::to_string(name.size()) + name;
}

std::string Igel::Mangler::mangle(std::vector<llvm::Type*> types, std::vector<BeContained*> typeNames,std::vector<bool> signage) {
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
        else name = (types[i]->isPointerTy()||types[i]->isIntegerTy(0)?"P":"") + mangle(typeNames[i]);
        c += !signage[i];
        if(c >= 0)str += c;
        else str += name;
    }
    if(types.empty())str += "v";
    return str;
}

std::string Igel::Mangler::mangle(BeContained* cont, std::vector<llvm::Type*> types, std::vector<BeContained*> typeNames,std::vector<bool> signage) {
    if(cont->name == "main")return "main";
    std::string str = "_Z";
    str += mangle(cont);
    if(str == "_Z6printf")return str;
    str += mangle(std::move(types),std::move(typeNames),std::move(signage));
    return str;
}
