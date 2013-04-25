#include "IR_Gen_Context.h"

namespace SC {

llvm::IRBuilder<> CG_Context::sBuilder(getGlobalContext());
llvm::Module* CG_Context::TheModule = NULL;
llvm::ExecutionEngine* CG_Context::TheExecutionEngine = NULL;
llvm::FunctionPassManager* CG_Context::TheFPM = NULL;
llvm::DataLayout* CG_Context::TheDataLayout = NULL;
std::hash_map<std::string, void*> CG_Context::sGlobalFuncSymbols;

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
	CG_Context::TheDataLayout = new DataLayout(*CG_Context::TheExecutionEngine->getDataLayout());
	CG_Context::TheFPM->add(CG_Context::TheDataLayout);
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


	// Set up the executing engine
	//
	// the sybmoll searching(e.g. for standard CRT) is disabled
	CG_Context::TheExecutionEngine->DisableSymbolSearching(true);


	return true;
}

void DestoryCodeGen()
{
	delete CG_Context::TheFPM;
	delete CG_Context::TheExecutionEngine;
}


CG_Context::CG_Context()
{
	mpParent = NULL;
	mpCurFunction = NULL;
	mpCurFuncRetBlk = NULL;
	mpRetValuePtr = NULL;
}

CG_Context* CG_Context::CreateChildContext(Function* pCurFunc, llvm::BasicBlock* pRetBlk, llvm::Value* pRetValuePtr)
{
	CG_Context* pRet = new CG_Context();
	pRet->mpParent = this;
	pRet->mpCurFunction = pCurFunc;
	pRet->mpCurFuncRetBlk = pRetBlk;
	pRet->mpRetValuePtr = pRetValuePtr;
	return pRet;
}

Function* CG_Context::GetCurrentFunc()
{
	return mpCurFunction;
}

llvm::BasicBlock* CG_Context::GetFuncRetBlk()
{
	return mpCurFuncRetBlk;
}

llvm::Value* CG_Context::GetRetValuePtr()
{
	return mpRetValuePtr;
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
	case VarType::kFloat8:
		return VectorType::get(SC_FLOAT_TYPE, 8);

	case VarType::kInt:
		return SC_INT_TYPE;
	case VarType::kInt2:
		return VectorType::get(SC_INT_TYPE, 2);
	case VarType::kInt3:
		return VectorType::get(SC_INT_TYPE, 3);
	case VarType::kInt4:
		return VectorType::get(SC_INT_TYPE, 4);
	case VarType::kInt8:
		return VectorType::get(SC_INT_TYPE, 8);
	case VarType::kBoolean:
		return SC_INT_TYPE; // Use integer to represent boolean value
	case VarType::kExternType:
		return llvm::PointerType::get(Type::getInt8Ty(getGlobalContext()), 0);
	case VarType::kVoid:
		return Type::getVoidTy(getGlobalContext());
	}

	return NULL;
}

int CG_Context::GetSizeOfLLVMType(VarType tp)
{
	llvm::Type* type = ConvertToLLVMType(tp);
	assert(type);
	return (int)TheDataLayout->getTypeAllocSize(type);
}

int CG_Context::GetAlignmentOfLLVMType(VarType tp)
{
	llvm::Type* type = ConvertToLLVMType(tp);
	assert(type);
	return (int)TheDataLayout->getABITypeAlignment(type);
}

llvm::Type* CG_Context::ConvertToPackedType(llvm::Type* srcType)
{
	llvm::Type* srcActualType = srcType;
	if (srcType->isPointerTy()) {
		llvm::PointerType* srcPtrType = dyn_cast<llvm::PointerType>(srcType);
		srcActualType = srcPtrType->getElementType();
	}
	llvm::Type* destType = NULL;

	if (srcActualType->isVectorTy()) {
		llvm::VectorType* vType = dyn_cast<llvm::VectorType>(srcActualType);
		llvm::Type* elemType = vType->getElementType();
		unsigned int elemCnt = vType->getNumElements();
		destType = llvm::ArrayType::get(elemType, elemCnt);
	}
	else if (srcActualType->isStructTy()) {
		llvm::StructType* structType = dyn_cast<llvm::StructType>(srcActualType);
		std::vector<llvm::Type*> newTypes;
		for (unsigned int i = 0; i < structType->getNumElements(); ++i) {
			newTypes.push_back(ConvertToPackedType(structType->getElementType(i)));
		}
		destType = llvm::StructType::create(getGlobalContext(), newTypes);
	}
	else if (srcActualType->isArrayTy()) {
		llvm::ArrayType* arrayType = dyn_cast<llvm::ArrayType>(srcActualType);
		llvm::Type* newElemType = ConvertToPackedType(arrayType->getElementType());
		destType = llvm::ArrayType::get(newElemType, arrayType->getNumElements());
	}
	else
		destType = srcActualType;

	if (srcType->isPointerTy()) {
		return llvm::PointerType::get(destType, 0);
	}
	else
		return destType;
}

void CG_Context::ConvertValueToPacked(llvm::Value* srcValue, llvm::Value* destPtr)
{
	llvm::Type* srcType = NULL;
	llvm::Value* srcActualValue = NULL;
	if (srcValue->getType()->isPointerTy()) {
		llvm::PointerType* srcPtrType = dyn_cast<llvm::PointerType>(srcValue->getType());
		srcType = srcPtrType->getElementType();
		srcActualValue = sBuilder.CreateLoad(srcValue);
	}
	else {
		srcType = srcValue->getType();
		srcActualValue = srcValue;
	}

	llvm::Value* destValuePtr = destPtr;

	if (srcType->isVectorTy()) {

		llvm::VectorType* vType = dyn_cast<llvm::VectorType>(srcType);
		for (unsigned int i = 0; i < vType->getNumElements(); ++i) {
			llvm::Value* idx = Constant::getIntegerValue(SC_INT_TYPE, APInt(sizeof(Int)*8, (uint64_t)i));
			llvm::Value* elemValue = sBuilder.CreateExtractElement(srcActualValue, idx);
			std::vector<llvm::Value*> indices(2);
			indices[1] = idx;
			indices[0] = Constant::getIntegerValue(SC_INT_TYPE, APInt(sizeof(Int)*8, (uint64_t)0));
			llvm::Value* elemPtr = sBuilder.CreateGEP(destValuePtr, indices);
			sBuilder.CreateStore(elemValue, elemPtr);
		}
		
	}
	else if (srcType->isArrayTy() || srcType->isStructTy()) {

		llvm::CompositeType* pCompType = dyn_cast<llvm::CompositeType>(srcType);
		unsigned int Idx = 0;
		while (1) {
			if (!pCompType->indexValid(Idx))
				break;

			llvm::Value* destIdx = Constant::getIntegerValue(SC_INT_TYPE, APInt(sizeof(Int)*8, (uint64_t)Idx));
			std::vector<llvm::Value*> indices(2);
			indices[1] = destIdx;
			indices[0] = Constant::getIntegerValue(SC_INT_TYPE, APInt(sizeof(Int)*8, (uint64_t)0));
			llvm::Value* destElemPtr = sBuilder.CreateGEP(destValuePtr, indices);

			std::vector<unsigned int> srcIdx(1);
			srcIdx[0] = Idx;
			llvm::Value* srcElemValue = sBuilder.CreateExtractValue(srcActualValue, srcIdx);

			ConvertValueToPacked(srcElemValue, destElemPtr);

			++Idx;
		}

	}
	else {
		sBuilder.CreateStore(srcActualValue, destValuePtr);
	}
}

llvm::Value* CG_Context::ConvertValueFromPacked(llvm::Value* srcValue, llvm::Type* destType)
{
	llvm::Type* srcType = NULL;
	llvm::Value* srcActualValue = NULL;
	if (srcValue->getType()->isPointerTy()) {
		llvm::PointerType* srcPtrType = dyn_cast<llvm::PointerType>(srcValue->getType());
		srcType = srcPtrType->getElementType();
		srcActualValue = sBuilder.CreateLoad(srcValue);
	}
	else {
		srcType = srcValue->getType();
		srcActualValue = srcValue;
	}

	llvm::Type* destActualType = destType;
	if (destType->isPointerTy()) {
		llvm::PointerType* destPtrType = dyn_cast<llvm::PointerType>(destType);
		destActualType = destPtrType->getElementType();
	}
	llvm::Value* destValuePtr = sBuilder.CreateAlloca(destActualType);

	if (destActualType->isVectorTy()) {

		llvm::Value* newVecValue = llvm::UndefValue::get(destActualType);
		llvm::VectorType* vType = dyn_cast<llvm::VectorType>(destActualType);
		assert(vType);
		assert(srcType->isArrayTy());
		for (unsigned int i = 0; i < vType->getNumElements(); ++i) {
			llvm::Value* idx = Constant::getIntegerValue(SC_INT_TYPE, APInt(sizeof(Int)*8, (uint64_t)i));
			std::vector<llvm::Value*> indices(2);
			indices[1] = idx;
			indices[0] = Constant::getIntegerValue(SC_INT_TYPE, APInt(sizeof(Int)*8, (uint64_t)0));

			std::vector<unsigned int> srcIdx(1);
			srcIdx[0] = i;
			llvm::Value* elemValue = sBuilder.CreateExtractValue(srcActualValue, srcIdx);

			newVecValue = sBuilder.CreateInsertElement(newVecValue, elemValue, idx);
			
		}
		sBuilder.CreateStore(newVecValue, destValuePtr);
	}
	else if (destActualType->isStructTy() || destActualType->isArrayTy()) {

		llvm::CompositeType* pCompType = dyn_cast<llvm::CompositeType>(destActualType);
		unsigned int Idx = 0;
		while (1) {
			if (!pCompType->indexValid(Idx))
				break;

			llvm::Value* destIdx = Constant::getIntegerValue(SC_INT_TYPE, APInt(sizeof(Int)*8, (uint64_t)Idx));
			std::vector<llvm::Value*> indices(2);
			indices[1] = destIdx;
			indices[0] = Constant::getIntegerValue(SC_INT_TYPE, APInt(sizeof(Int)*8, (uint64_t)0));
			llvm::Value* destElemPtr = sBuilder.CreateGEP(destValuePtr, indices);
			llvm::PointerType* destElemPtrType = dyn_cast<llvm::PointerType>(destElemPtr->getType());

			std::vector<unsigned int> srcIdx(1);
			srcIdx[0] = Idx;
			llvm::Value* srcElemValue = sBuilder.CreateExtractValue(srcActualValue, srcIdx);

			llvm::Value* convertedSrcValue = ConvertValueFromPacked(srcElemValue, destElemPtrType->getElementType());
			sBuilder.CreateStore(convertedSrcValue, destElemPtr);

			++Idx;
		}

	}
	else {
		sBuilder.CreateStore(srcActualValue, destValuePtr);
	}

	if (srcValue->getType()->isPointerTy())
		return destValuePtr;
	else
		return sBuilder.CreateLoad(destValuePtr);
}

llvm::Function* CG_Context::CreateFunctionWithPackedArguments(const KSC_FunctionDesc& fDesc)
{
	llvm::Function* wrapperF = NULL;
	std::vector<llvm::Type*> wrapperF_argTypes;
	std::vector<llvm::Type*> orgArgTypes;
	int Idx = 0;
	for (Function::arg_iterator AI = fDesc.F->arg_begin(); AI != fDesc.F->arg_end(); ++AI, ++Idx) {
		llvm::Type* argType = AI->getType();
		orgArgTypes.push_back(argType);
		wrapperF_argTypes.push_back(fDesc.needJITPacked[Idx] ? SC::CG_Context::ConvertToPackedType(argType) : argType);
		
	}
	FunctionType *FT = FunctionType::get(SC::CG_Context::ConvertToPackedType(fDesc.F->getReturnType()), wrapperF_argTypes, false);
	wrapperF = Function::Create(FT, Function::ExternalLinkage, fDesc.F->getName() + "_packed", CG_Context::TheModule);

	BasicBlock *BB = BasicBlock::Create(getGlobalContext(), "entry_packed", wrapperF);
	sBuilder.SetInsertPoint(BB);

	std::vector<llvm::Value*> args;
	// Convert the non-packed arguments to packed ones
	//
	Idx = 0;
	for (Function::arg_iterator AI = wrapperF->arg_begin(); AI != wrapperF->arg_end(); ++AI, ++Idx) {
		args.push_back(fDesc.needJITPacked[Idx] ? ConvertValueFromPacked(AI, orgArgTypes[Idx]) : AI);
	}
	// Invoke the target function
	//
	llvm::Value* retValue = sBuilder.CreateCall(fDesc.F, args);
	// Convert back the packed arguments to non-packed ones(if they're passed-by-reference)
	//
	Idx = 0;
	for (Function::arg_iterator wrapperAI = wrapperF->arg_begin(); wrapperAI != wrapperF->arg_end(); ++wrapperAI, ++Idx) {
		if (wrapperAI->getType()->isPointerTy()) {
			assert(args[Idx]->getType()->isPointerTy());
			if (fDesc.needJITPacked[Idx])
				ConvertValueToPacked(sBuilder.CreateLoad(args[Idx]), wrapperAI);
		}
	}

	if (!fDesc.F->getReturnType()->isVoidTy()) {
		llvm::Value* retValuePtr = sBuilder.CreateAlloca(wrapperF->getReturnType());
		ConvertValueToPacked(retValue, retValuePtr);
	
		sBuilder.CreateRet(sBuilder.CreateLoad(retValuePtr));
	}
	else
		sBuilder.CreateRetVoid();

	return wrapperF;
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
			if (llvmType) {
				if (pVarDef->GetArrayCnt() > 0) {
					llvm::Type* arrayType = llvm::ArrayType::get(llvmType, pVarDef->GetArrayCnt());
					ret = TmpB.CreateAlloca(arrayType, 0, name.c_str());
				}
				else 
					ret = TmpB.CreateAlloca(llvmType, 0, name.c_str());
			}
		}
		else {
			llvm::Type* llvmType = CG_Context::ConvertToLLVMType(pVarDef->GetVarType());
			if (llvmType) {
				if (pVarDef->GetArrayCnt() > 0) {
					llvm::Type* arrayType = llvm::ArrayType::get(llvmType, pVarDef->GetArrayCnt());
					ret = TmpB.CreateAlloca(arrayType, 0, name.c_str());
				}
				else
					ret = TmpB.CreateAlloca(llvmType, 0, name.c_str());
			}
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
		int arraySize = 0;
		VarType elemSC_Type = pStructDef->GetElementType(i, elemStructDef, arraySize);
		if (elemSC_Type == VarType::kStructure) {
			elemTypes[i] = GetStructType(elemStructDef);
		}
		else
			elemTypes[i] = ConvertToLLVMType(elemSC_Type);

		if (arraySize > 0)
			elemTypes[i] = ArrayType::get(elemTypes[i], arraySize);
		assert(elemTypes[i]);
	}
	ArrayRef<Type*> typeArray(&elemTypes[0], elemCnt);

	llvm::Type* ret = StructType::create(getGlobalContext(), typeArray, pStructDef->GetStructureName().c_str());
	mStructTypes[pStructDef] = ret;
	return ret;
}

void CG_Context::AddFunctionDecl(const std::string& funcName, llvm::Function* pF)
{
	assert(mFuncDecls.find(funcName) == mFuncDecls.end());
	mFuncDecls[funcName] = pF;
}

llvm::Function* CG_Context::GetFuncDeclByName(const std::string& funcName)
{
	if (mFuncDecls.find(funcName) != mFuncDecls.end())
		return mFuncDecls[funcName];
	else
		return mpParent ? mpParent->GetFuncDeclByName(funcName) : NULL;
}

bool RootDomain::CompileToIR(CG_Context* pPredefine, KSC_ModuleDesc& mouduleDesc)
{
	CG_Context* cgCtx = pPredefine->CreateChildContext(pPredefine->GetCurrentFunc(), pPredefine->GetFuncRetBlk(), pPredefine->GetRetValuePtr());
	for (int i = 0; i < (int)mExpressions.size(); ++i) {
		llvm::Value* value = mExpressions[i]->GenerateCode(cgCtx);

		Exp_FunctionDecl* pFuncDecl = dynamic_cast<Exp_FunctionDecl*>(mExpressions[i]);
		if (pFuncDecl && pFuncDecl->HasBody()) {
			llvm::Function* funcValue = llvm::dyn_cast_or_null<llvm::Function>(value);
			KSC_FunctionDesc* pFuncDesc = new KSC_FunctionDesc;
			pFuncDesc->F = funcValue;
			for (int ai = 0; ai < pFuncDecl->GetArgumentCnt(); ++ai)
				pFuncDesc->needJITPacked.push_back(pFuncDecl->GetArgumentDesc(ai)->needJITPacked ? 1 : 0);
			pFuncDecl->ConvertToDescription(*pFuncDesc, *cgCtx);
			mouduleDesc.mFunctionDesc[pFuncDecl->GetFunctionName()] = pFuncDesc;
		}

		Exp_StructDef* pStructDef = dynamic_cast<Exp_StructDef*>(mExpressions[i]);
		if (pStructDef) {
			KSC_StructDesc* pStructDesc = new KSC_StructDesc;
			pStructDef->ConvertToDescription(*pStructDesc, *cgCtx);
			mouduleDesc.mGlobalStructures[pStructDef->GetStructureName()] = pStructDesc;
		}
	}
	delete cgCtx;
	CG_Context::TheModule->dump();
	return true;
}

llvm::Value* CG_Context::CastValueType(llvm::Value* srcValue, VarType srcType, VarType destType)
{

	bool srcIorF = !IsFloatType(srcType);
	int srcElemCnt = TypeElementCnt(srcType);
	bool destIorF = !IsFloatType(destType);
	int destElemCnt = TypeElementCnt(destType);

	if (srcElemCnt == 1 && destElemCnt == 1) {
		//
		// For scalar types
		//
		if (srcType == VarType::kBoolean && destType == VarType::kBoolean) {
			llvm::Value* falseValue = Constant::getIntegerValue(SC_INT_TYPE, APInt(sizeof(Int)*8, (uint64_t)0));
			llvm::Value* trueValue = Constant::getIntegerValue(SC_INT_TYPE, APInt(sizeof(Int)*8, (uint64_t)1));
			return sBuilder.CreateSelect(srcValue, trueValue, falseValue);
		}

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
	if (srcElemCnt < destElemCnt) 
		return NULL;
	else
		return srcValue;
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
		else if (opStr == "||" || opStr == "|") 
			return sBuilder.CreateOr(pL, R_Value); // boolean values are treated as i1 integer
		else if (opStr == "&&" || opStr == "&") 
			return sBuilder.CreateAnd(pL, R_Value); // boolean values are treated as i1 integer

	}
	
	return NULL;
}

}