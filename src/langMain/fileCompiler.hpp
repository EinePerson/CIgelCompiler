//
// Created by igel on 01.08.24.
//

#ifndef FILECOMPILER_H
#define FILECOMPILER_H

#include <utility>
#include <clang/Frontend/CompilerInstance.h>
#include <clang/CodeGen/CodeGenAction.h>
#include <clang/Driver/Compilation.h>
#include <clang/Driver/Driver.h>
#include <clang/Driver/ToolChain.h>
#include <clang/Frontend/CompilerInstance.h>
#include <clang/Frontend/CompilerInvocation.h>
#include <clang/Frontend/FrontendActions.h>
#include <clang/Frontend/TextDiagnosticPrinter.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/Support/raw_ostream.h>
#include <clang/Lex/PreprocessorOptions.h>
#include <llvm/Support/TargetSelect.h>
#include <llvm/TargetParser/Host.h>

#include "llvm/ExecutionEngine/Orc/LLJIT.h"

#include "../CompilerInfo/InfoParser.h"
#include "../cxx_extension/CXX_Parser.h"
#include "codeGen/Generator.h"
#include "parsing/Indexer.h"
#include "parsing/Parser.h"
#include "parsing/PreParser.h"

class FileCompiler {
private:
    bool isInit = false;
    bool own;
public:
    std::vector<SrcFile*> files;
    std::unordered_map<std::string,SrcFile*> file_table;
    //std::vector<Header*> header;
    std::unordered_map<std::string,Header*> header_table;
    Tokenizer m_tokenizer;
    Indexer m_indexer;
    PreParser m_preParser;
    Parser m_parser;
    Generator* m_gen;

    FileCompiler(Generator* gen,std::vector<SrcFile*> files,std::unordered_map<std::string,SrcFile*> file_table,std::unordered_map<std::string,Header*> header_table = {}) :
    files(files),file_table(file_table)/*,header(header)*/,header_table(header_table),m_indexer(file_table,header_table),m_gen(gen) {
        isInit = true;
        own = false;
    }

    FileCompiler(InternalInfo* info,std::vector<SrcFile*> files,std::unordered_map<std::string,SrcFile*> file_table,std::unordered_map<std::string,Header*> header_table = {}) :
    files(files),file_table(file_table)/*,header(header)*/,header_table(header_table),m_indexer(file_table,header_table),m_gen(new Generator(info)) {
        isInit = true;
        own = true;
    }

    explicit FileCompiler(InternalInfo* info) {
        own = true;
        m_gen = new Generator(info);
    }

    virtual ~FileCompiler() {
        if(own)delete m_gen;
    }

    void init(std::vector<SrcFile*> files, const std::unordered_map<std::string,SrcFile*> &file_table
        ,std::vector<Header*> header = {}, const std::unordered_map<std::string,Header*> &header_table = {}) {
        this->files = files;
        this->file_table = file_table;
        //this->header = header;
        this->header_table = header_table;
        m_indexer.setup(file_table,header_table);
        isInit = true;
    }

    void checkInit() const {
        if(!isInit) {
            std::cerr << "FileCompiler has to be initialized before usage" << std::endl;
            exit(EXIT_FAILURE);
        }
    }

    void tokenize() {
        checkInit();
        for(const auto file : files) {
            if(!file->tokens.empty())continue;
            file->tokens = m_tokenizer.tokenize(file->fullName);
        }
    }

    void preParse() {
        checkInit();
        for(const auto file : files){
            m_indexer.index(file);
            m_preParser.parse(file);
        }
    }

    void parse() {
        checkInit();
        for (auto file : files) {
            m_parser.parseProg(file);
        }
    }

    void preGen() {
        checkInit();
        for(const auto file: files){
            m_gen->create(file);
            m_gen->generateSigs();
        }
    }

    void gen(const std::optional<std::function<void(SrcFile* file,FileCompiler* cmp)>> &fileCallback = {}) {
        checkInit();
        if(fileCallback) {
            for(const auto file:files){
                m_gen->setup(file);
                m_gen->generate();
                fileCallback.value()(file,this);
            }
        }else {
           for(const auto file:files){
                m_gen->setup(file);
                m_gen->generate();
            }
        }
    }

    virtual void compile() {
        tokenize();
        preParse();
        parse();
        preGen();
        gen();
    }
};

class JITFileCompiler : public FileCompiler {
    std::vector<const char*> headers;
    std::vector<SrcFile*> nonLive;
    std::vector<SrcFile*> live;
    //std::unique_ptr<LLVMContext> cntxtPtr;
public:
    std::unique_ptr<orc::LLJIT> jit;

    explicit JITFileCompiler(InternalInfo* info):FileCompiler((Generator*) nullptr,{},{}){
        m_gen = new Generator(info);
        InitializeAllTargetInfos();
        InitializeAllTargets();
        InitializeAllTargetMCs();
        InitializeAllAsmParsers();
        InitializeAllAsmPrinters();

        ExitOnError ExitOnErr;
        auto JTMB = ExitOnErr(orc::JITTargetMachineBuilder::detectHost());
        JTMB.setCodeModel(CodeModel::Small);

        jit = ExitOnErr(orc::LLJITBuilder().setJITTargetMachineBuilder(std::move(JTMB)).create());

        addLib("/lib64/libclang.so");
    }

    JITFileCompiler(InternalInfo* info,std::vector<SrcFile*> files,std::vector<SrcFile*> liveFiles,std::unordered_map<std::string,SrcFile*> file_table
        ,std::unordered_map<std::string,Header*> header_table = {}) :
    /*cntxtPtr(std::make_unique<LLVMContext>()),*/FileCompiler(info,join(files,liveFiles),std::move(file_table),std::move(header_table)),live(liveFiles),nonLive(files){
        InitializeAllTargetInfos();
        InitializeAllTargets();
        InitializeAllTargetMCs();
        InitializeAllAsmParsers();
        InitializeAllAsmPrinters();

        ExitOnError ExitOnErr;
        auto JTMB = ExitOnErr(orc::JITTargetMachineBuilder::detectHost());
        JTMB.setCodeModel(CodeModel::Small);

        jit = ExitOnErr(orc::LLJITBuilder().setJITTargetMachineBuilder(std::move(JTMB)).create());

        addLib("/lib64/libclang.so");
    }
private:

    std::vector<SrcFile*> join(std::vector<SrcFile*> first,std::vector<SrcFile*> second) {
        std::vector<SrcFile*> ret(first);
        ret.insert(ret.end(),second.begin(), second.end());
        return ret;
    }

public:

    void setLiveFiles(std::vector<SrcFile*> live) {
        this->live = live;
    }

    void link() {
        for (auto file : live)m_gen->setup(file);
        orc::ThreadSafeContext cnt(std::move(m_gen->m_contxt));
        for (auto src_file : live) {
            m_gen->setup(src_file);
            if(auto ret = jit->addIRModule(orc::ThreadSafeModule(std::move(m_gen->m_module),cnt))) {
                outs() << ret;
                exit(EXIT_FAILURE);
            }
        }
    }

    void compileHeaders() {
        if(headers.empty())return;

        auto diagOptions = new clang::DiagnosticOptions();
        clang::CompilerInstance compiler;
        auto invocation = std::make_shared<clang::CompilerInvocation>();
        llvm::IntrusiveRefCntPtr<clang::DiagnosticIDs> DiagID(new clang::DiagnosticIDs());

        diagOptions->IgnoreWarnings = true;
        clang::DiagnosticsEngine Diags(DiagID, diagOptions,new clang::TextDiagnosticPrinter(llvm::errs(), diagOptions));
        Diags.setIgnoreAllWarnings(true);
        auto incl = CXX_Parser::getClangIncludePaths();
        std::vector<const char*> filenames;
        filenames.insert(filenames.end(),headers.begin(), headers.end());
        filenames.push_back("-x");
        filenames.push_back("c++");
        filenames.push_back("-w");
        for (const auto& basic_string : incl) {
            filenames.push_back(basic_string.c_str());
        }
        clang::CompilerInvocation::CreateFromArgs(*invocation,filenames,Diags);
        invocation->getPreprocessorOpts().UsePredefines = true;
        invocation->getLangOpts().CPlusPlus = true;
        invocation->getLangOpts().RTTI = true;

        //Ownership is handed to the compiler and freed(or at least deleting it here causes a memory corruption)
        auto diagEngine = new clang::DiagnosticsEngine(DiagID, diagOptions,new clang::TextDiagnosticPrinter(llvm::errs(), diagOptions));
        diagEngine->setIgnoreAllWarnings(true);
        compiler.setDiagnostics(diagEngine);

        std::shared_ptr<clang::TargetOptions> pto = std::make_shared<clang::TargetOptions>();
        pto->Triple = llvm::sys::getDefaultTargetTriple();
        clang::TargetInfo *pti = clang::TargetInfo::CreateTargetInfo(compiler.getDiagnostics(), pto);
        compiler.setTarget(pti);

        compiler.createFileManager();
        compiler.createSourceManager(compiler.getFileManager());
        compiler.setInvocation(invocation);
        compiler.createPreprocessor(clang::TU_Complete);

        std::string cmd = "clang -dM -E - < /dev/null";
        std::string result = Process::exec(cmd.c_str());

        std::string line;
        std::istringstream iss(result);
        while (std::getline(iss, line)) {
            std::string name(line.begin() + line.find_first_of(' ') + 1,line.begin() + line.find_last_of(' '));
            std::string val(line.begin() + line.find_last_of(' ') + 1,line.end());
            std::string macro(name + "=" + val);
            compiler.getPreprocessorOpts().addMacroDef(macro);
        }

        std::unique_ptr<clang::CodeGenAction> action(new clang::EmitLLVMOnlyAction());
        compiler.ExecuteAction(*action);
        auto mod = action->takeModule();
        if (!mod) {
            llvm::errs() << "Error generating module\n";
            exit(EXIT_FAILURE);
        }

        if(auto ret = jit->addIRModule(orc::ThreadSafeModule(std::move(mod),std::unique_ptr<LLVMContext>(action->takeLLVMContext())))) {
            outs() << ret;
            exit(EXIT_FAILURE);
        }
    }

    void genNonLive(const std::optional<std::function<void(SrcFile* file,FileCompiler* cmp)>> &fileCallback = {}) {
        checkInit();
        if(fileCallback) {
            for(const auto file:nonLive){
                m_gen->setup(file);
                m_gen->generate();
                fileCallback.value()(file,this);
            }
        }else {
            for(const auto file:nonLive){
                m_gen->setup(file);
                m_gen->generate();
            }
        }
    }

    void genLive() {
        checkInit();
        for (auto file : live) {
            m_gen->setup(file);
            m_gen->generate();
        }
    }

    void compile() override {
        tokenize();
        preParse();
        parse();
        preGen();
        genLive();
        compileHeaders();
        link();
    }

    int execute(const std::string &entryPointName) {
        auto exp = jit->lookup(entryPointName);
        if(!exp) {
            outs() << exp.takeError();
            exit(EXIT_FAILURE);
        }
        auto mainFunc = exp.get().toPtr<int()>();
        return mainFunc();
    }

    void addHeader(const char* headerPath) {
        headers.push_back(headerPath);
    }

    void addLib(std::string name) {
        ExitOnError exitOnErr;
        auto err = jit->addObjectFile(exitOnErr(errorOrToExpected(MemoryBuffer::getFileAsStream(name))));
        exitOnErr(std::move(err));
    }
};

#endif //FILECOMPILER_H
