//
// Created by igel on 15.07.24.
//

#include "CXX_Parser.h"

#include <cmath>
#include <utility>
#include <llvm/MC/TargetRegistry.h>
#include <clang/Tooling/CommonOptionsParser.h>
#include <clang/Tooling/Tooling.h>
#include <clang/Frontend/FrontendActions.h>
#include <clang/Frontend/CompilerInstance.h>
#include <clang/AST/AST.h>
#include <clang/AST/RecursiveASTVisitor.h>

#include "../util/Mangler.h"

#include <clang/AST/Type.h>
#include <clang/Frontend/FrontendAction.h>
#include <llvm/IR/Type.h>
#include <memory>
#include <ranges>
#include <unordered_set>

#include "../langMain/codeGen/Generator.h"

using namespace clang;
using namespace clang::tooling;

ASTVisitor::ASTVisitor(CXX_Parser *parser) : parser(parser) {
}

bool ASTVisitor::VisitFunctionDecl(clang::FunctionDecl *function_decl) {
    function_decl->addAttr(UsedAttr::Create(function_decl->getASTContext()));
    auto val = parseFunc(function_decl);
    auto par = getTypeFromDecl(function_decl->getParent());
    val->member = false;
    if(par) {
        if(auto nmsp = dynamic_cast<NamesSpace*>(par.value())) {
            nmsp->funcs.push_back(val);
            parser->m_header->funcs[val->mangle()] = val;
            val->member = true;
        }
    }
    parser->m_header->funcs[val->mangle()] = val;
    return true;
}

bool ASTVisitor::VisitNamespaceDecl(clang::NamespaceDecl *namespace_decl) {
    auto nmsp = new NamesSpace;
    nmsp->_extern = true;
    nmsp->name = namespace_decl->getName();
    nmsp->contType = getTypeFromDecl(namespace_decl->getParent());
    parser->m_header->nameTypeMap[nmsp->mangle()] = nmsp;
    return true;
}

bool ASTVisitor::VisitEnumDecl(clang::EnumDecl* enum_decl) {
    auto _enum = new Enum;
    _enum->_extern = true;
    _enum->name = enum_decl->getName();
    uint i = 0;
    for(auto it = enum_decl->enumerator_begin();it != enum_decl->enumerator_end();++it) {
        _enum->values.push_back(it->getName().str());
        _enum->valueIdMap[it->getName().str()] = i;
        i++;
    }
    parser->m_header->nameTypeMap[_enum->mangle()] = _enum;
    return true;
}

bool ASTVisitor::VisitCXXRecordDecl(clang::CXXRecordDecl *record_decl) {
    IgType* igType = nullptr;
    uint type = 0;
    type += record_decl->methods().empty();
    bool intfQual = true;
    for (auto method : record_decl->methods()) {
        if(!method->isPureVirtual() && !method->getNameAsString().contains("operator") && !method->getNameAsString().contains(record_decl->getNameAsString())) {
            intfQual = false;
            break;
        }
    }
    type += (record_decl->fields().empty() * intfQual) * 2;
    if(type == 1 && record_decl->isStruct()) {
        auto str = parseStruct(record_decl);
        if(!str) {
            CXX_Ext::err("Expected struct");
        }
        str.value()->contType = getTypeFromDecl(record_decl->getParent());
        parser->m_header->nameTypeMap[str.value()->mangle()] = str.value();
        parser->m_header->structs[str.value()->mangle()] = str.value();
        igType = str.value();
    }else if(type == 2) {
        auto intf = parseInterface(record_decl);
        if(!intf) {
            CXX_Ext::err("Expected interface");
        }
        intf.value()->contType = getTypeFromDecl(record_decl->getParent());
        parser->m_header->nameTypeMap[intf.value()->mangle()] = intf.value();
        parser->m_header->interfaces[intf.value()->mangle()] = intf.value();
        igType = intf.value();
    }else {
        auto clz = parseClass(record_decl);
        if(!clz) {
            CXX_Ext::err("Expected class");
        }
        parser->m_header->nameTypeMap[clz.value()->mangle()] = clz.value();
        parser->m_header->classes[clz.value()->mangle()] = clz.value();
        igType = clz.value();
    }

    return true;
}

Igel::Access ASTVisitor::convert(clang::AccessSpecifier acc) {
    switch (acc) {
        case AS_private: {
            Igel::Access acc;
            acc.setPrivate();
            return acc;
        }
        case AS_protected: {
            Igel::Access acc;
            acc.setProtected();
            return acc;
        }
        default: {
            Igel::Access acc;
            acc.setPublic();
            return acc;
        }
    }
}

llvm::Type * ASTVisitor::convert(const clang::Type *type) {
    if(!type->getAs<BuiltinType>()) {
        //TODO return correct type
        if(type->isClassType())return llvm::PointerType::get(*Generator::m_contxt,0);
        else if(type->isStructureType()) {
            //type->getAsStructureType().
        }
        return llvm::PointerType::get(*Generator::m_contxt,0);
    }
    switch (type->getAs<BuiltinType>()->getKind()) {
        case BuiltinType::Bool:
            return llvm::Type::getInt1Ty(*Generator::m_contxt);
        case BuiltinType::Char8:
            return llvm::Type::getInt8Ty(*Generator::m_contxt);
        case BuiltinType::Short:
            return llvm::Type::getInt16Ty(*Generator::m_contxt);
        case BuiltinType::Int:
            return llvm::Type::getInt32Ty(*Generator::m_contxt);
        case BuiltinType::Long:
            return llvm::Type::getInt64Ty(*Generator::m_contxt);

        case BuiltinType::Half:
            return llvm::Type::getHalfTy(*Generator::m_contxt);
        case BuiltinType::Float:
            return llvm::Type::getFloatTy(*Generator::m_contxt);
        case BuiltinType::Double:
            return llvm::Type::getDoubleTy(*Generator::m_contxt);
        case BuiltinType::Void:
            return llvm::Type::getVoidTy(*Generator::m_contxt);
        default:
            return llvm::PointerType::get(*Generator::m_contxt,0);
    }
}

IgFunction * ASTVisitor::parseFunc(clang::FunctionDecl* method_decl) {
    auto func = new IgFunction;
    method_decl->addAttr(UsedAttr::CreateImplicit(method_decl->getASTContext()));
    func->externC = method_decl->isExternC();
    auto str = method_decl->getNameInfo().getAsString();
    if(method_decl->getNameInfo().getAsString().starts_with('~')) {
        func->name = "D2";
        func->destructor = true;
    }else func->name = method_decl->getNameInfo().getAsString();
    func->abstract = method_decl->isPureVirtual();
    func->acc = convert(method_decl->getAccess());
    func->constructor = false;
    if(method_decl->getParent()) {
        if(auto rec = clang::dyn_cast<CXXRecordDecl>(method_decl->getParent())) {
            if(rec->getNameAsString() == method_decl->getNameInfo().getAsString())func->constructor = true;

            func->member = true;
            func->paramType.push_back(llvm::PointerType::get(*Generator::m_contxt,0));
            func->signage.push_back(false);
            func->paramName.emplace_back("this");
            func->paramTypeName.push_back(parser->m_header->findContained(method_decl->getNameInfo().getAsString()).value_or(nullptr));
        }
    }
    func->final = method_decl->isVirtualAsWritten();
    func->member = method_decl->getParent();
    for (uint i = 0;i < method_decl->parameters().size();i++) {
        func->paramType.push_back(convert(method_decl->parameters()[i]->getType().getTypePtr()));
        func->signage.push_back(method_decl->parameters()[i]->getType()->isSignedIntegerOrEnumerationType());
        func->paramName.push_back(method_decl->parameters()[i]->getName().str());
        func->paramTypeName.push_back(parser->m_header->findContained(method_decl->parameters()[i]->getType()->getPointeeType().getAsString()).value_or(nullptr));
    }
    func->returnSigned = method_decl->getReturnType()->isSignedIntegerType();
    func->_return = convert(method_decl->getReturnType().getTypePtr());

    auto tst = method_decl->getDeclaredReturnType().getTypePtr()->getPointeeType().getAsString();
    if(!method_decl->getDeclaredReturnType().getTypePtr()->isPointerType())func->retTypeName = "";
    else func->retTypeName = method_decl->getDeclaredReturnType().getTypePtr()->getPointeeType().getAsString();

    auto val = getTypeFromDecl(method_decl->getParent());
    if(val) {
        if(auto clz = dynamic_cast<Class*>(val.value())) {
            func->supper = clz;
        }
    }
    /*if(func->constructor) {
        func->contType = func->contType.has_value()?func->contType.value()->contType:{};
    }*/
    return func;
}

std::optional<IgType *> ASTVisitor::getTypeFromDecl(clang::DeclContext *cntxt) {
    //if(!cntxt)return {};
    if(cntxt->isNamespace()) {
        return parser->m_header->findContained(cast<NamespaceDecl>(cntxt)->getQualifiedNameAsString());
    }else if(cntxt->isRecord()) {
        return parser->m_header->findContained(cast<CXXRecordDecl>(cntxt)->getQualifiedNameAsString());
    }else if(cntxt->getDeclKind() == Decl::Kind::Enum) {
        return parser->m_header->findContained(cast<EnumDecl>(cntxt)->getQualifiedNameAsString());
    }
    return {};
}

clang::CXXRecordDecl * ASTVisitor::getOriginalBaseClass(const clang::CXXMethodDecl *method_decl) {
    auto base = const_cast<CXXRecordDecl*>(method_decl->getParent());
    for (auto overridden_method : method_decl->overridden_methods()) base = getOriginalBaseClass(overridden_method);
    return base;
}

std::optional<Struct *> ASTVisitor::parseStruct(clang::CXXRecordDecl *decl) {
    auto str = new Struct;
    str->_extern = true;
    uint i = 0;
    for (auto field : decl->fields()) {
        str->varIds.push_back(field->getName().str());
        str->types.push_back(convert(field->getType().getTypePtr()));
        str->varIdMs[field->getName().str()] = i;
        str->finals[field->getName().str()] = false;
        str->signage.push_back(field->getType()->isSignedIntegerType());
        str->typeName.push_back(getTypeFromDecl(field->getDeclContext()).value_or(nullptr));
        i++;
    }

    str->name = decl->getName();
    return str;
}

std::optional<Class *> ASTVisitor::parseClass(clang::CXXRecordDecl *decl) {
    auto clz = new Class;
    clz->_extern = true;
    clz->name = decl->getName();
    clz->contType = getTypeFromDecl(decl->getParent());
    if(decl->hasDefinition()) {
        for (auto base : decl->bases()) {
            auto tst = base.getType().getAsString();
            auto ext = parser->m_header->findContained(base.getType().getAsString());
            if(!ext) {
                CXX_Ext::err("Unknown Type: " + base.getType().getAsString());
            }
            if(auto intf = dynamic_cast<Interface*>(ext.value())) {
                clz->implementing.push_back(intf);
            }else if(auto clzE = dynamic_cast<Class*>(ext.value())) {
                if(!clz->extending)clz->extending = clzE;
                else {
                    CXX_Ext::err("Cannot extend more then one clazz in: " + clz->mangle());
                }
            }else {
                CXX_Ext::err("Invalid type: " + base.getType().getAsString());
            }
        }

        clz->abstract = decl->isAbstract();
    }

    //uint funcCount = origFuncCount;
    //uint superCount = 0;
    bool virt = false;
    uint offset = clz->extending?std::ceil(Generator::dataLayout.getTypeSizeInBits(clz->extending.value()->strType) / 64.0f):0;
    std::unordered_map<clang::CXXRecordDecl*, uint> funcIDs;
    std::unordered_map<clang::CXXRecordDecl*, uint> superIDs;
    for (auto method : decl->methods()) {
        auto par = method->getThisType().getAsString();
        IgFunction* func = parseFunc(method);
        func->contType = clz;
        func->supper = clz;
        if(method->isVirtual()) {
            auto clazzDecl = getOriginalBaseClass(method);
            if(!superIDs.contains(clazzDecl)) {
                if(superIDs.empty()) {
                    superIDs[clazzDecl] = 0;
                }else {
                    using pair_type = decltype(superIDs)::value_type;
                    uint max = std::max_element(superIDs.begin(),superIDs.end(),[](const pair_type p1,const pair_type p2) {
                        return p1<p2;
                    })->second;
                    if(!(getTypeFromDecl(clazzDecl) && dynamic_cast<ContainableType*>(getTypeFromDecl(clazzDecl).value())))CXX_Ext::err("Unknown type: " + clazzDecl->getNameAsString());;
                    superIDs[clazzDecl] = max + offset;
                    funcIDs[clazzDecl] = clazzDecl->hasDefinition()?(clazzDecl->hasUserDeclaredDestructor() * 2):0;
                }
            }

            for(uint i = 0; i < 1 + func->destructor;i++) {
                clz->funcs.push_back(func);
                clz->funcs.back()->member = true;
                clz->funcMap[superIDs[clazzDecl]][funcIDs[clazzDecl]] = func;
                clz->funcIdMs[func->mangle()] = std::make_pair(superIDs[clazzDecl],funcIDs[clazzDecl]);
                funcIDs[clazzDecl]++;
            }
            virt = true;
        }else {
            //TODO add calling for non-virtual functions (maybe next update)

        }

        if(func && func->constructor) {
            func->name = clz->name;
            func->contType = clz->contType;
            if(func->paramName.size() == 1 && func->paramName[0] == "this")clz->defaulfConstructor = func;
            clz->constructors.push_back(func);
            clz->hasConstructor = true;
        }
    }

    uint i = virt;
    if(clz->extending) {
        clz->types.push_back(clz->extending.value()->strType);
    }

    /*if(virt) {
        clz->varNames.push_back("");
        if(!clz->extending)clz->types.push_back(llvm::PointerType::get(*Generator::m_contxt,0));
        clz->finals[""] = false;
        Igel::Access acc;
        acc.setPublic();
        clz->varAccesses[""] = acc;
        clz->varTypeNames[""] = "";
    }*/
    for(uint j = 0;j < clz->implementing.size() + virt;j++) {
        clz->varNames.push_back("");
        if(!clz->extending)clz->types.push_back(llvm::PointerType::get(*Generator::m_contxt,0));
        clz->finals[""] = false;
        Igel::Access acc;
        acc.setPublic();
        clz->varAccesses[""] = acc;
        clz->varTypeNames[""] = "";
    }
    for (auto field : decl->fields()) {
        clz->varNames.push_back(field->getName().str());
        clz->types.push_back(convert(field->getType().getTypePtr()));
        clz->varIdMs[field->getName().str()] = i;
        clz->finals[field->getName().str()] = false;
        clz->varAccesses[field->getName().str()] = convert(field->getAccess());
        auto typeName = getTypeFromDecl(field->getDeclContext());
        clz->varTypeNames[field->getName().str()] = typeName?typeName.value()->mangle():"";
        i++;
    }

    clz->strType = llvm::StructType::create(*Generator::m_contxt,clz->types,clz->mangle());

    return clz;
}

std::optional<Interface *> ASTVisitor::parseInterface(clang::CXXRecordDecl *decl) {
    auto intf = new Interface;
    intf->_extern = true;
    for (auto method : decl->methods()) {
        if(method->isVirtual()) {
            intf->funcs.push_back(parseFunc(method));
        }
    }

    intf->name = decl->getName();
    return intf;
}

Consumer::Consumer(CXX_Parser *parser) : parser(parser),visitor(parser) {
}

ActionFactory::ActionFactory(CXX_Parser *parser) : parser(parser) {
}

std::unique_ptr<clang::FrontendAction> ActionFactory::create() {
    return std::make_unique<IgFrontendAction>(parser);
}

IgFrontendAction::IgFrontendAction(CXX_Parser* parser) : parser(parser) {
}

std::unique_ptr<clang::ASTConsumer> IgFrontendAction::CreateASTConsumer(clang::CompilerInstance &CI, llvm::StringRef file) {
    return std::make_unique<Consumer>(parser);
}

CXX_Parser::CXX_Parser(Header* header) : m_header(header) {

}

Header* CXX_Parser::parseHeader() {
    std::vector<std::string> incl = getClangIncludePaths();
    std::vector<std::string> args = {"-x","c++","-Wno-pragma-once-outside-header"};
    args.insert(args.end(),incl.begin(), incl.end());
    FixedCompilationDatabase db(".",args);
    ClangTool tool(db,{m_header->fullName});
    auto fact = new ActionFactory(this);
    tool.run(fact);
    delete fact;
    return m_header;
}
