#pragma once

#include "parser_AST_Gen.h"

#include <llvm/DerivedTypes.h>
#include <llvm/ExecutionEngine/ExecutionEngine.h>
#include <llvm/ExecutionEngine/JIT.h>
#include <llvm/IRBuilder.h>
#include <llvm/LLVMContext.h>
#include <llvm/Module.h>
#include <llvm/PassManager.h>
#include <llvm/Analysis/Verifier.h>
#include <llvm/Analysis/Passes.h>
#include <llvm/DataLayout.h>
#include <llvm/Transforms/Scalar.h>
#include <llvm/Support/TargetSelect.h>
#include <llvm/Intrinsics.h>

using namespace llvm;

#ifdef WANT_DOUBLE_FLOAT
#define SC_FLOAT_TYPE Type::getDoubleTy(getGlobalContext())
#else
#define SC_FLOAT_TYPE Type::getFloatTy(getGlobalContext())
#endif

#define SC_INT_TYPE Type::getInt32Ty(getGlobalContext())

namespace SC {

bool InitializeCodeGen();
void DestoryCodeGen();

class CG_Context
{
private:
	CG_Context* mpParent;
	llvm::Function* mpCurFunction;
	llvm::BasicBlock* mpCurFuncRetBlk;
	llvm::Value* mpRetValuePtr;

	std::hash_map<std::string, llvm::Value*> mVariables;
	std::hash_map<std::string, llvm::Function*> mFuncDecls;
	std::hash_map<const Exp_StructDef*, llvm::Type*> mStructTypes;
	
public:
	static llvm::Module *TheModule;
	static llvm::ExecutionEngine* TheExecutionEngine;
	static llvm::FunctionPassManager* TheFPM;
	static llvm::IRBuilder<> sBuilder;
	static std::hash_map<std::string, void*> sGlobalFuncSymbols;
	static std::hash_map<std::string, llvm::Intrinsic::ID> mIntrinsicFuncs;

public:
	static llvm::Type* ConvertToLLVMType(VarType tp);

	static bool IsIntrinsicFunc(const std::string& funcName);
	static llvm::Value* CreateIntrinsicCall(const std::string& funcName, std::vector<llvm::Value*>& values);

	CG_Context();
	llvm::Function* GetCurrentFunc();
	llvm::BasicBlock* GetFuncRetBlk();
	llvm::Value* GetRetValuePtr();

	llvm::Value* GetVariableValue(const std::string& name, bool includeParent);
	llvm::Value* GetVariablePtr(const std::string& name, bool includeParent);
	llvm::Value* NewVariable(const Exp_VarDef* pVarDef, llvm::Value* pRefPtr);
	llvm::Type* GetStructType(const Exp_StructDef* pStructDef);
	llvm::Type* NewStructType(const Exp_StructDef* pStructDef);
	void AddFunctionDecl(const std::string& funcName, llvm::Function* pF);
	llvm::Function* GetFuncDeclByName(const std::string& funcName);
	CG_Context* CreateChildContext(Function* pCurFunc, llvm::BasicBlock* pRetBlk, llvm::Value* pRetValuePtr);

	llvm::Value* CastValueType(llvm::Value* srcValue, VarType srcType, VarType destType);

	llvm::Value* CreateBinaryExpression(const std::string& opStr, 
		llvm::Value* pL, llvm::Value* pR, VarType Ltype, VarType Rtype);
};

} // namespace SC