#include "IR_Gen_Context.h"

namespace SC {

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


void* RootDomain::JIT_Function(const std::string& funcName)
{
	Exp_FunctionDecl* pFuncDecl = GetFunctionDeclByName(funcName);
	if (!pFuncDecl)
		return NULL;

	llvm::Function* funcValue = (llvm::Function*)(pFuncDecl->GenerateCode(new CG_Context()));
	if (!funcValue)
		return NULL;

	void *retFuncPtr = CG_Context::TheExecutionEngine->getPointerToFunction(funcValue);
	return retFuncPtr;
}

}