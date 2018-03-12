#include "node.h"
#include "codegen.h"
#include "parser.hpp"
#include <llvm/IR/Module.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/Type.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/DerivedTypes.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/CallingConv.h>
#include <llvm/Bitcode/BitcodeReader.h>
#include <llvm/Bitcode/BitcodeWriter.h>
#include <llvm/IR/Verifier.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/ExecutionEngine/GenericValue.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/IR/LegacyPassManager.h>
#include "llvm/IR/IRPrintingPasses.h"

using namespace std;
using namespace llvm;
using namespace clike;

LLVMContext llvmContext;

LLVMContext& getGlobalContext() {
    return llvmContext;
}

CodeGenContext::CodeGenContext() {
    mModule = new Module("main", getGlobalContext());
}

/* Compile the AST into a module */
void CodeGenContext::generateCode(NBlock& root) {
    std::cout << "Generating code...\n";

    /* Create the top level interpreter function to call as entry */
    vector <Type*> argTypes;
    FunctionType* ftype = FunctionType::get(Type::getVoidTy(getGlobalContext()), argTypes, false);
    mMainFunction = Function::Create(ftype, GlobalValue::InternalLinkage, "main", mModule);
    BasicBlock* bblock = BasicBlock::Create(getGlobalContext(), "entry", mMainFunction, 0);

    /* Push a new variable/block context */
    pushBlock(bblock);
    root.codeGen(*this); /* emit bytecode for the toplevel block */
    ReturnInst::Create(getGlobalContext(), bblock);
    popBlock();

    /* Print the bytecode in a human-readable format
        to see if our program compiled properly
        */
    std::cout << "Code is generated.\n";
    llvm::legacy::PassManager pm;
    pm.add(createPrintModulePass(outs()));
    pm.run(*mModule);
}

/* Executes the AST by running the main function */
void CodeGenContext::runCode() {
    std::cout << "Running code...\n";
    llvm::legacy::PassManager PM;
    PM.run(*mModule);
    std::cout << "Code was run.\n";
}

/* Returns an LLVM type based on the identifier */
static Type* typeOf(const NIdentifier& type) {
    if (type.name.compare("int") == 0) {
        return Type::getInt64Ty(getGlobalContext());
    }
    else if (type.name.compare("double") == 0) {
        return Type::getDoubleTy(getGlobalContext());
    }
    return Type::getVoidTy(getGlobalContext());
}

/* -- Code Generation -- */

Value* NInteger::codeGen(CodeGenContext& context) {
    std::cout << "Creating integer: " << value << std::endl;
    return ConstantInt::get(Type::getInt64Ty(getGlobalContext()), value, true);
}

Value* NDouble::codeGen(CodeGenContext& context) {
    std::cout << "Creating double: " << value << std::endl;
    return ConstantFP::get(Type::getDoubleTy(getGlobalContext()), value);
}

Value* NIdentifier::codeGen(CodeGenContext& context) {
    std::cout << "Creating identifier reference: " << name << std::endl;
    if (context.locals().find(name) == context.locals().end()) {
        std::cerr << "undeclared variable " << name << std::endl;
        return NULL;
    }
    return new LoadInst(context.locals()[name], "", false, context.currentBlock());
}

Value* NMethodCall::codeGen(CodeGenContext& context) {
    Function* function = context.mModule->getFunction(id.name.c_str());
    if (function == NULL) {
        std::cerr << "no such function " << id.name << std::endl;
    }
    std::vector <Value*> args;
    ExpressionList::const_iterator it;
    for (it = arguments.begin(); it != arguments.end(); it++) {
        args.push_back((**it).codeGen(context));
    }
    CallInst* call = CallInst::Create(function, args, "", context.currentBlock());
    std::cout << "Creating method call: " << id.name << std::endl;
    return call;
}

Value* NBinaryOperator::codeGen(CodeGenContext& context) {
    std::cout << "Creating binary operation " << op << std::endl;
    Instruction::BinaryOps instr;
    switch (op) {
    case TPLUS:
        instr = Instruction::Add;
        goto math;

    case TMINUS:
        instr = Instruction::Sub;
        goto math;

    case TMUL:
        instr = Instruction::Mul;
        goto math;

    case TDIV:
        instr = Instruction::SDiv;
        goto math;

        /* TODO comparison */
    }

    return NULL;
math:
    return BinaryOperator::Create(instr, lhs.codeGen(context),
                                  rhs.codeGen(context), "", context.currentBlock());
}

Value* NAssignment::codeGen(CodeGenContext& context) {
    std::cout << "Creating assignment for " << lhs.name << std::endl;
    if (context.locals().find(lhs.name) == context.locals().end()) {
        std::cerr << "undeclared variable " << lhs.name << std::endl;
        return NULL;
    }
    return new StoreInst(rhs.codeGen(context), context.locals()[lhs.name], false, context.currentBlock());
}

Value* NBlock::codeGen(CodeGenContext& context) {
    StatementList::const_iterator it;
    Value* last = NULL;
    for (it = statements.begin(); it != statements.end(); it++) {
        /* std::cout << "Generating code for " << typeid(**it).name() << std::endl; */
        last = (**it).codeGen(context);
    }
    std::cout << "Creating block" << std::endl;
    return last;
}

Value* NExpressionStatement::codeGen(CodeGenContext& context) {
    /* std::cout << "Generating code for " << typeid(expression).name() << std::endl; */
    return expression.codeGen(context);
}

Value* NVariableDeclaration::codeGen(CodeGenContext& context) {
    std::cout << "Creating variable declaration " << type.name << " " << id.name << std::endl;
    AllocaInst* alloc = new AllocaInst(typeOf(type), 0, nullptr, id.name, context.currentBlock());

    context.locals()[id.name] = alloc;
    if (assignmentExpr != NULL) {
        NAssignment assn(id, * assignmentExpr);

        assn.codeGen(context);
    }
    return alloc;
}

Value* NFunctionDeclaration::codeGen(CodeGenContext& context) {
    vector <Type*> argTypes;
    VariableList::const_iterator it;
    for (it = arguments.begin(); it != arguments.end(); it++) {
        argTypes.push_back(typeOf((**it).type));
    }
    FunctionType* ftype = FunctionType::get(typeOf(type), argTypes, false);
    Function* function = Function::Create(ftype, GlobalValue::InternalLinkage, id.name.c_str(), context.mModule);
    BasicBlock* bblock = BasicBlock::Create(getGlobalContext(), "entry", function, 0);

    context.pushBlock(bblock);

    for (it = arguments.begin(); it != arguments.end(); it++) {
        (**it).codeGen(context);
    }

    block.codeGen(context);
    ReturnInst::Create(getGlobalContext(), bblock);

    context.popBlock();
    std::cout << "Creating function: " << id.name << std::endl;
    return function;
}

