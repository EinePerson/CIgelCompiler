//
// Created by igel on 09.02.24.
//

#ifndef MANGLER_H
#define MANGLER_H
#include <string>

#include "../types.h"

namespace Igel {
    enum ManglerType {
        ITANIUM,
        NONE,
    };
    class Mangler{
        struct Mangled {
            bool mangled;
            BeContained* cont;
            std::string name;
        };

    virtual std::string mangleTypeNameImpl(BeContained* cont) = 0;
    virtual std::string mangleNameImpl(BeContained* cont) = 0;
    ///\brief member indicates weather the first parameter is the object the function is called on
    virtual std::string mangleImpl(BeContained* cont,bool member,bool constructor,bool destructor) = 0;
    virtual std::string mangleImpl(std::string name) = 0;
    ///\brief member indicates weather the first parameter is the object the function is called on
    virtual std::string mangleImpl(std::vector<llvm::Type*> types,std::vector<BeContained*> typeNames,std::vector<bool> signage,bool member,bool constructor) = 0;
    ///\brief member indicates weather the first parameter is the object the function is called on
    virtual std::string mangleImpl(BeContained* cont,std::vector<llvm::Type*> types,std::vector<BeContained*> typeNames,std::vector<bool> signage,bool member,bool constructor,bool destructor) = 0;

    static Mangler* currentMangler;
    static Mangler* noMangle;

    public:
        virtual ~Mangler() = default;

        static void init(Mangler* mangler) {
            currentMangler = mangler;
        }

        static bool isInit() {
            return currentMangler != nullptr;
        }

        static void init(ManglerType type);

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



    class ItaniumMangler final : public Mangler{
        friend class Mangler;

        std::string mangleTypeNameImpl(BeContained *cont) override;

        std::string mangleNameImpl(BeContained *cont) override;

        std::string mangleImpl(BeContained *cont, bool member, bool constructor, bool destructor) override;

        std::string mangleImpl(std::string name) override;

        std::string mangleImpl(std::vector<llvm::Type *> types, std::vector<BeContained *> typeNames,
            std::vector<bool> signage, bool member, bool constructor) override;

        std::string mangleImpl(BeContained *cont, std::vector<llvm::Type *> types, std::vector<BeContained *> typeNames,
            std::vector<bool> signage, bool member, bool constructor, bool destructor) override;
    };

    class NoMangler final : public Mangler {
        std::string mangleNameImpl(BeContained *cont) override;

        std::string mangleImpl(BeContained *cont, bool member, bool constructor, bool destructor) override;

        std::string mangleImpl(std::string name) override;

        std::string mangleImpl(std::vector<llvm::Type *> types, std::vector<BeContained *> typeNames,
            std::vector<bool> signage, bool member, bool constructor) override;

        std::string mangleImpl(BeContained *cont, std::vector<llvm::Type *> types, std::vector<BeContained *> typeNames,
            std::vector<bool> signage, bool member, bool constructor, bool destructor) override;

        std::string mangleTypeNameImpl(BeContained *cont) override;
    };
}
#endif //MANGLER_H
