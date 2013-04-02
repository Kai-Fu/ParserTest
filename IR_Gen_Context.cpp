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

llvm::Value* CG_Context::GetVariableValue(const std::string& name, bool includeParent)
{
	llvm::Value* ptr = GetVariablePtr(name, includeParent);
	return ptr ? sBuilder.CreateLoad(ptr, name) : NULL;
}

llvm::Value* CG_Context::GetVariablePtr(const std::string& name, bool includeParent)
{
	std::hash_map<std::string, llvm::Value*>::iterator it = mVariables.find(name);
	if (it == mVariables.end() && includeParent)
		return mpParent ? mpParent->GetVariablePtr(name, true) : NULL;
	else
		return it == mVariables.end() ? NULL : it->second;
}

llvm::Value* CG_Context::NewVariable(const Exp_VarDef* pVarDef, llvm::Value* pRefPtr)
{
	assert(mpCurFunction);
	std::string name = pVarDef->GetVarName().ToStdString();
	if (mVariables.find(name) != mVariables.end())
		return NULL;
	IRBuilder<> TmpB(&mpCurFunction->getEntryBlock(),
                 mpCurFunction->getEntryBlock().begin());
	llvm::Value* ret = pRefPtr;
	if (!pRefPtr) {
		if (pVarDef->GetVarType() == VarType::kStructure) {
			llvm::Type* llvmType = GetStructType(pVarDef->GetStructDef());
			if (llvmType)
				ret = TmpB.CreateAlloca(llvmType, 0, name.c_str());
		}
		else {
			llvm::Type* llvmType = CG_Context::ConvertToLLVMType(pVarDef->GetVarType());
			if (llvmType)
				ret = TmpB.CreateAlloca(llvmType, 0, name.c_str());
		}
	}
	
	if (ret) mVariables[name] = ret;
	return ret;
}

llvm::Type* CG_Context::GetStructType(const Exp_StructDef* pStructDef)
{
	std::hash_map<const Exp_StructDef*, llvm::Type*>::iterator it = mStructTypes.find(pStructDef);
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


bool RootDomain::JIT_Compile()
{
	CG_Context* cgCtx = new CG_Context();
	for (int i = 0; i < (int)mExpressions.size(); ++i) {
		llvm::Value* value = mExpressions[i]->GenerateCode(cgCtx);
		Exp_FunctionDecl* pFuncDecl = dynamic_cast<Exp_FunctionDecl*>(mExpressions[i]);
		if (pFuncDecl) {
			llvm::Function* funcValue = llvm::dyn_cast_or_null<llvm::Function>(value);
			llvm::verifyFunction(*funcValue);

			void *funcPtr = CG_Context::TheExecutionEngine->getPointerToFunction(funcValue);
			mJITedFuncPtr[pFuncDecl->GetFunctionName()] = funcPtr;
		}
	}

	CG_Context::TheModule->dump();
	return true;
}

void* RootDomain::GetFuncPtrByName(const std::string& funcName)
{
	std::hash_map<std::string, void*>::iterator it = mJITedFuncPtr.find(funcName);
	if (it != mJITedFuncPtr.end())
		return it->second;
	else
		return NULL;
}

llvm::Value* CG_Context::CastValueType(llvm::Value* srcValue, VarType srcType, VarType destType)
{

	bool srcIorF = IsIntegerType(srcType);
	int srcElemCnt = TypeElementCnt(srcType);
	bool destIorF = IsIntegerType(destType);
	int destElemCnt = TypeElementCnt(destType);

	if (srcElemCnt == 1 && destElemCnt == 1) {
		//
		// For scalar types
		//

		if (srcIorF != destIorF) {
			if (destIorF)
				return sBuilder.CreateFPToSI(srcValue, SC_INT_TYPE);
			else
				return sBuilder.CreateSIToFP(srcValue, SC_FLOAT_TYPE);
		}
		else
			return srcValue;
	}
	else if (srcElemCnt == destElemCnt) {
		//
		// For vector types of the same element count
		//

		if (srcIorF != destIorF) {
			if (destIorF) {
				llvm::Type* destType = ConvertToLLVMType(MakeType(destIorF, destElemCnt));
				return sBuilder.CreateFPToSI(srcValue, destType);
			}
			else {
				llvm::Type* destType = ConvertToLLVMType(MakeType(destIorF, destElemCnt));
				return sBuilder.CreateSIToFP(srcValue, destType);
			}
		}
		else {
			return srcValue;
		}
	}
	else if (srcElemCnt > destElemCnt) {

		//
		// For vector types of different element count
		//

		llvm::Value* destValue = NULL;
		llvm::Type* destType = ConvertToLLVMType(MakeType(destIorF, destElemCnt));
		llvm::SmallVector<Constant*, 4> Idxs;
		for (int i = 0; i < destElemCnt; ++i) 
			Idxs.push_back(sBuilder.getInt32(i));
		llvm::Value* truncatedValue = sBuilder.CreateShuffleVector(srcValue, llvm::UndefValue::get(srcValue->getType()), llvm::ConstantVector::get(Idxs));
		if (srcIorF != destIorF) {
			if (destIorF) {
				llvm::Type* destType = ConvertToLLVMType(MakeType(destIorF, destElemCnt));
				return sBuilder.CreateFPToSI(truncatedValue, destType);
			}
			else {
				llvm::Type* destType = ConvertToLLVMType(MakeType(destIorF, destElemCnt));
				return sBuilder.CreateSIToFP(truncatedValue, destType);
			}
		}
		else
			return truncatedValue;
	}
	else
		return NULL;
}

llvm::Value* CG_Context::CreateBinaryExpression(const std::string& opStr, 
		llvm::Value* pL, llvm::Value* pR, VarType Ltype, VarType Rtype)
{
	llvm::Value* R_Value = CastValueType(pR, Rtype, Ltype);
	assert(R_Value);
	if (IsFloatType(Ltype)) {
		// Generate instruction for float type
		if (opStr == "+") 
			return sBuilder.CreateFAdd(pL, R_Value);
		else if (opStr == "-") 
			return sBuilder.CreateFSub(pL, R_Value);
		else if (opStr == "*") 
			return sBuilder.CreateFMul(pL, R_Value);
		else if (opStr == "/") 
			return sBuilder.CreateFDiv(pL, R_Value);
		else if (opStr == ">") 
			return sBuilder.CreateFCmpOGT(pL, R_Value);
		else if (opStr == ">=") 
			return sBuilder.CreateFCmpOGE(pL, R_Value);
		else if (opStr == "<") 
			return sBuilder.CreateFCmpOLT(pL, R_Value);
		else if (opStr == "<=") 
			return sBuilder.CreateFCmpOLE(pL, R_Value);
		else if (opStr == "==") 
			return sBuilder.CreateFCmpOEQ(pL, R_Value);
	}
	else {
		// Generate instruction for integer type
		if (opStr == "+") 
			return sBuilder.CreateAdd(pL, R_Value);
		else if (opStr == "-") 
			return sBuilder.CreateSub(pL, R_Value);
		else if (opStr == "*") 
			return sBuilder.CreateMul(pL, R_Value);
		else if (opStr == "/") 
			return sBuilder.CreateSDiv(pL, R_Value);
		else if (opStr == ">") 
			return sBuilder.CreateICmpSGT(pL, R_Value);
		else if (opStr == ">=") 
			return sBuilder.CreateICmpSGE(pL, R_Value);
		else if (opStr == "<") 
			return sBuilder.CreateICmpSLT(pL, R_Value);
		else if (opStr == "<=") 
			return sBuilder.CreateICmpSLE(pL, R_Value);
		else if (opStr == "==") 
			return sBuilder.CreateICmpEQ(pL, R_Value);
	}
	
	return NULL;
}

}