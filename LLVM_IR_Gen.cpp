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

using namespace llvm;

#ifdef WANT_DOUBLE_FLOAT
#define SC_FLOAT_TYPE Type::getDoubleTy(getGlobalContext())
#else
#define SC_FLOAT_TYPE Type::getFloatTy(getGlobalContext())
#endif

#define SC_INT_TYPE Type::getInt32Ty(getGlobalContext())

namespace SC {

class CG_Context
{
private:
	CG_Context* mpParent;
	Function* mpCurFunction;
	std::hash_map<std::string, llvm::Value*> mVariables;
	std::hash_map<const Exp_StructDef*, llvm::Type*> mStructTypes;
public:
	static llvm::Module *TheModule;
	static llvm::ExecutionEngine* TheExecutionEngine;
	static llvm::FunctionPassManager* TheFPM;

public:
	static llvm::IRBuilder<> sBuilder;

	static llvm::Type* ConvertToLLVMType(VarType tp);
	static AllocaInst* CreateEntryBlockAlloca(Function *TheFunction, llvm::Type* argType, const std::string &argName);

	CG_Context();
	Function* GetCurrentFunc();

	llvm::Value* GetVariable(const std::string& name, bool includeParent);
	llvm::Value* NewVariable(const Exp_VarDef* pVarDef);
	llvm::Type* GetStructType(const Exp_StructDef* pStructDef);
	llvm::Type* NewStructType(const Exp_StructDef* pStructDef);
	CG_Context* CreateChildContext(Function* pCurFunc);
};

llvm::IRBuilder<> CG_Context::sBuilder(getGlobalContext());
llvm::Module* CG_Context::TheModule = NULL;
llvm::ExecutionEngine* CG_Context::TheExecutionEngine = NULL;
llvm::FunctionPassManager* CG_Context::TheFPM = NULL;

bool InitializeCodeGen()
{
	llvm::InitializeNativeTarget();
	LLVMContext &llvmCtx = llvm::getGlobalContext();
	CG_Context::TheModule = new Module("Kai's Shader Compiler", llvmCtx);
	std::string ErrStr;
	CG_Context::TheExecutionEngine = EngineBuilder(CG_Context::TheModule).setErrorStr(&ErrStr).create();
	if (!CG_Context::TheExecutionEngine) {
		return false;
	}

	CG_Context::TheFPM = new llvm::FunctionPassManager(CG_Context::TheModule);

	// Set up the optimizer pipeline.  Start with registering info about how the
	// target lays out data structures.
	CG_Context::TheFPM->add(new DataLayout(*CG_Context::TheExecutionEngine->getDataLayout()));
	// Provide basic AliasAnalysis support for GVN.
	CG_Context::TheFPM->add(createBasicAliasAnalysisPass());
	// Promote allocas to registers.
	CG_Context::TheFPM->add(createPromoteMemoryToRegisterPass());
	// Do simple "peephole" optimizations and bit-twiddling optzns.
	CG_Context::TheFPM->add(createInstructionCombiningPass());
	// Reassociate expressions.
	CG_Context::TheFPM->add(createReassociatePass());
	// Eliminate Common SubExpressions.
	CG_Context::TheFPM->add(createGVNPass());
	// Simplify the control flow graph (deleting unreachable blocks, etc).
	CG_Context::TheFPM->add(createCFGSimplificationPass());

	CG_Context::TheFPM->doInitialization();

	return true;
}

void DestoryCodeGen()
{
	delete CG_Context::TheFPM;
	delete CG_Context::TheExecutionEngine;
	delete CG_Context::TheModule;
}


CG_Context::CG_Context()
{
	mpParent = NULL;
	mpCurFunction = NULL;
}

CG_Context* CG_Context::CreateChildContext(Function* pCurFunc)
{
	CG_Context* pRet = new CG_Context();
	pRet->mpParent = this;
	pRet->mpCurFunction = pCurFunc;
	return pRet;
}

Function* CG_Context::GetCurrentFunc()
{
	return mpCurFunction;
}

AllocaInst* CreateEntryBlockAlloca(Function *pFunc, llvm::Type* argType, const std::string &argName) 
{
	llvm::IRBuilder<> TmpB(&pFunc->getEntryBlock(), pFunc->getEntryBlock().begin());
	return TmpB.CreateAlloca(argType, 0, argName.c_str());
}

llvm::Type* CG_Context::ConvertToLLVMType(VarType tp)
{
	switch (tp) {
	case VarType::kFloat:
		return SC_FLOAT_TYPE;
	case VarType::kFloat2:
		return VectorType::get(SC_FLOAT_TYPE, 2);
	case VarType::kFloat3:
		return VectorType::get(SC_FLOAT_TYPE, 3);
	case VarType::kFloat4:
		return VectorType::get(SC_FLOAT_TYPE, 4);
		break;

	case VarType::kInt:
		return SC_INT_TYPE;
	case VarType::kInt2:
		return VectorType::get(SC_INT_TYPE, 2);
	case VarType::kInt3:
		return VectorType::get(SC_INT_TYPE, 3);
		break;
	case VarType::kInt4:
		return VectorType::get(SC_INT_TYPE, 4);
		break;
	case VarType::kBoolean:
		return SC_INT_TYPE; // Use integer to represent boolean value
	case VarType::kVoid:
		return Type::getVoidTy(getGlobalContext());
	}

	return NULL;
}

llvm::Value* CG_Context::GetVariable(const std::string& name, bool includeParent)
{
	std::hash_map<std::string, llvm::Value*>::iterator it = mVariables.find(name);
	if (it == mVariables.end() && includeParent)
		return mpParent ? mpParent->GetVariable(name, true) : NULL;
	else
		return it->second;
}

llvm::Value* CG_Context::NewVariable(const Exp_VarDef* pVarDef)
{
	assert(mpCurFunction);
	std::string name = pVarDef->GetVarName().ToStdString();
	if (mVariables.find(name) != mVariables.end())
		return NULL;
	IRBuilder<> TmpB(&mpCurFunction->getEntryBlock(),
                 mpCurFunction->getEntryBlock().begin());
	llvm::Value* ret = NULL;
	if (pVarDef->GetVarType() == VarType::kStructure) {
		llvm::Type* llvmType = GetStructType(pVarDef->GetStructDef());
		if (llvmType)
			ret = TmpB.CreateAlloca(llvmType, 0, name.c_str());
		return ret;
	}
	else {
		llvm::Type* llvmType = CG_Context::ConvertToLLVMType(pVarDef->GetVarType());
		if (llvmType)
			ret = TmpB.CreateAlloca(llvmType, 0, name.c_str());
		return ret;
	}
}

llvm::Type* CG_Context::GetStructType(const Exp_StructDef* pStructDef)
{
	std::hash_map<const Exp_StructDef*, llvm::Type*>::iterator it = mStructTypes.begin();
	if (it != mStructTypes.end())
		return it->second;
	else
		return mpParent ? mpParent->GetStructType(pStructDef) : NULL;
}

llvm::Type* CG_Context::NewStructType(const Exp_StructDef* pStructDef)
{
	int elemCnt = pStructDef->GetElementCount();
	std::vector<llvm::Type*> elemTypes(elemCnt);
	for (int i = 0; i < elemCnt; ++i) {
		const Exp_StructDef* elemStructDef = NULL;
		VarType elemSC_Type = pStructDef->GetElementType(i, elemStructDef);
		if (elemSC_Type == VarType::kStructure) {
			elemTypes[i] = GetStructType(elemStructDef);
		}
		else
			elemTypes[i] = ConvertToLLVMType(elemSC_Type);
		assert(elemTypes[i]);
	}
	ArrayRef<Type*> typeArray(&elemTypes[0], elemCnt);

	llvm::Type* ret = StructType::create(getGlobalContext(), typeArray);
	mStructTypes[pStructDef] = ret;
	return ret;
}

llvm::Value* Expression::GenerateCode(CG_Context* context)
{
	// Should not reach here
	assert(0);
	return NULL;
}

llvm::Value* Exp_Constant::GenerateCode(CG_Context* context)
{
	if (mIsFromFloat) 
		return ConstantFP::get(getGlobalContext(), APFloat((Float)mValue));
	else
		return Constant::getIntegerValue(SC_INT_TYPE, APInt(sizeof(Int)*8, (uint64_t)mValue));
}

llvm::Value* Exp_VarDef::GenerateCode(CG_Context* context)
{
	std::string varName = mVarName.ToStdString();
	assert(context->GetVariable(varName, false) == NULL);
	return context->NewVariable(this);
}

llvm::Value* Exp_StructDef::GenerateCode(CG_Context* context)
{
	context->NewStructType(this);
	return NULL;
}

llvm::Value* Exp_TrueOrFalse::GenerateCode(CG_Context* context)
{
	return Constant::getIntegerValue(SC_INT_TYPE, APInt(sizeof(Int)*8, mValue ? 1 : 0));
}

llvm::Value* Exp_VariableRef::GenerateCode(CG_Context* context)
{
	return context->GetVariable(mVariable.ToStdString(), true);
}

llvm::Value* Exp_UnaryOp::GenerateCode(CG_Context* context)
{
	if (mOpType == "!")
		return CG_Context::sBuilder.CreateNot(mpExpr->GenerateCode(context));
	else if (mOpType == "-")
		return CG_Context::sBuilder.CreateNeg(mpExpr->GenerateCode(context));
	else {
		assert(0);
		return NULL;
	}
}

llvm::Value* Exp_BinaryOp::GenerateCode(CG_Context* context)
{
	llvm::Value* VL = mpLeftExp->GenerateCode(context);
	llvm::Value* VR = mpRightExp->GenerateCode(context);
	if (!VL || !VR)
		return NULL;
	if (mOperator == "=") {
		CG_Context::sBuilder.CreateStore(VL, VR);
		return VL;
	}

	return NULL;
}

llvm::Value* Exp_FunctionDecl::GenerateCode(CG_Context* context)
{
	// handle the argument types
	std::vector<llvm::Type*> funcArgTypes(mArgments.size());
	for (int i = 0; i < (int)mArgments.size(); ++i) {

		VarType scType = mArgments[i].typeInfo.type;
		if (scType == VarType::kStructure) {
			funcArgTypes[i] = context->GetStructType(mArgments[i].typeInfo.pStructDef);
		}
		else {
			funcArgTypes[i] = context->ConvertToLLVMType(scType);
		}
	}
	// handle the return type
	llvm::Type* retType = NULL;
	if (mReturnType == VarType::kStructure) {
		retType = context->GetStructType(mpRetStruct);
	}
	else 
		retType = context->ConvertToLLVMType(mReturnType);

	FunctionType *FT = FunctionType::get(retType, funcArgTypes, false);
	Function *F = Function::Create(FT, Function::ExternalLinkage, mFuncName, CG_Context::TheModule);

	// set names for all arguments
	{
		unsigned Idx = 0;
		for (Function::arg_iterator AI = F->arg_begin(); Idx != mArgments.size(); ++AI, ++Idx) 
			AI->setName(mArgments[Idx].token.ToStdString());
	}

	CG_Context* funcGC_ctx = context->CreateChildContext(F);
	// Create a new basic block to start insertion into, this basic blokc is a must for a function.
	BasicBlock *BB = BasicBlock::Create(getGlobalContext(), mFuncName + "_entry", F);
	CG_Context::sBuilder.SetInsertPoint(BB);

	Function::arg_iterator AI = F->arg_begin();
	for (unsigned Idx = 0, e = funcArgTypes.size(); Idx != e; ++Idx, ++AI) {
		Exp_VarDef* pVarDef = dynamic_cast<Exp_VarDef*>(mExpressions[Idx]);
		assert(pVarDef);
		llvm::Value* funcArg = pVarDef->GenerateCode(funcGC_ctx);
		// Store the input argument's value in the the local variables.
		CG_Context::sBuilder.CreateStore(AI, funcArg);
	}

	// The last expression of the function domain should be the function body(which is a child domain)
	CodeDomain* pFuncBody = dynamic_cast<CodeDomain*>(mExpressions[mArgments.size()]);
	assert(pFuncBody);
	pFuncBody->GenerateCode(funcGC_ctx);

	return F;
}

llvm::Value* CodeDomain::GenerateCode(CG_Context* context)
{
	CG_Context* domain_ctx = context->CreateChildContext(context->GetCurrentFunc());
	for (int i = 0; i < (int)mExpressions.size(); ++i) {
		mExpressions[i]->GenerateCode(domain_ctx);
	}
	return NULL; // the domain doesn't have the value to return
}

llvm::Value* Exp_FuncRet::GenerateCode(CG_Context* context)
{
	llvm::Value* retVal = mpRetValue->GenerateCode(context);
	assert(retVal);
	return CG_Context::sBuilder.CreateRet(retVal);
}

} // namespace SC