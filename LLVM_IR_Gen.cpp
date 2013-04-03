#include "parser_AST_Gen.h"
#include "IR_Gen_Context.h"

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

namespace SC {

llvm::Value* Expression::GenerateCode(CG_Context* context) const
{
	// Should not reach here
	assert(0);
	return NULL;
}

llvm::Value* Exp_Constant::GenerateCode(CG_Context* context) const
{
	if (mIsFromFloat) 
		return ConstantFP::get(getGlobalContext(), APFloat((Float)mValue));
	else
		return Constant::getIntegerValue(SC_INT_TYPE, APInt(sizeof(Int)*8, (uint64_t)mValue, true));
}

llvm::Value* Exp_VarDef::GenerateCode(CG_Context* context) const
{
	std::string varName = mVarName.ToStdString();
	llvm::Value* varPtr = context->NewVariable(this, NULL);
	if (mpInitValue) {
		llvm::Value* initValue = mpInitValue->GenerateCode(context);
		CG_Context::sBuilder.CreateStore(initValue, varPtr);
	}
	return varPtr;
}

llvm::Value* Exp_StructDef::GenerateCode(CG_Context* context) const
{
	context->NewStructType(this);
	return NULL;
}

llvm::Value* Exp_TrueOrFalse::GenerateCode(CG_Context* context) const
{
	return Constant::getIntegerValue(SC_INT_TYPE, APInt(/* Only one bit*/1, mValue ? 1 : 0));
}

llvm::Value* Exp_VariableRef::GenerateCode(CG_Context* context) const
{
	if (mpDef->GetVarType() == VarType::kBoolean) {
		llvm::Value* intValue = context->GetVariableValue(mVariable.ToStdString(), true);
		llvm::Value* falseValue = Constant::getIntegerValue(SC_INT_TYPE, APInt(sizeof(Int)*8, (uint64_t)0));
		return CG_Context::sBuilder.CreateICmpNE(intValue, falseValue);
	}
	else
		return context->GetVariableValue(mVariable.ToStdString(), true);
}

llvm::Value* Exp_UnaryOp::GenerateCode(CG_Context* context) const
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


void Exp_VariableRef::GenerateAssignCode(CG_Context* context, llvm::Value* pValue) const
{
	llvm::Value* varPtr = context->GetVariablePtr(mpDef->GetVarName().ToStdString(), true);
	if (mpDef->GetVarType() == VarType::kBoolean) {
		llvm::Value* falseValue = Constant::getIntegerValue(SC_INT_TYPE, APInt(sizeof(Int)*8, (uint64_t)0));
		llvm::Value* trueValue = Constant::getIntegerValue(SC_INT_TYPE, APInt(sizeof(Int)*8, (uint64_t)1));
		llvm::Value* thisBoolValue = CG_Context::sBuilder.CreateSelect(pValue, trueValue, falseValue);
		CG_Context::sBuilder.CreateStore(thisBoolValue, varPtr);
	}
	else
		CG_Context::sBuilder.CreateStore(pValue, varPtr);
}


llvm::Value* Exp_BinaryOp::GenerateCode(CG_Context* context) const
{
	llvm::Value* VR = mpRightExp->GenerateCode(context);
	if (!VR)
		return NULL;
	if (mOperator == "=") {
		llvm::Value* castedValue = context->CastValueType(VR, mpRightExp->GetCachedTypeInfo().type, mpLeftExp->GetCachedTypeInfo().type);
		mpLeftExp->GenerateAssignCode(context, castedValue);
		llvm::Value* VL = mpLeftExp->GenerateCode(context);
		return VL;
	}
	else {
		llvm::Value* VL = mpLeftExp->GenerateCode(context);
		Exp_ValueEval::TypeInfo LtypeInfo, RtypeInfo;
		LtypeInfo = mpLeftExp->GetCachedTypeInfo();
		RtypeInfo = mpRightExp->GetCachedTypeInfo();
		assert(!LtypeInfo.pStructDef);

		return context->CreateBinaryExpression(mOperator, VL, VR,LtypeInfo.type, RtypeInfo.type); 
	}

	return NULL;
}

llvm::Value* Exp_FunctionDecl::GenerateCode(CG_Context* context) const
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

		if (mArgments[i].isByRef) 
			funcArgTypes[i] =  llvm::PointerType::get(funcArgTypes[i], 0);
	
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
	for (int Idx = 0, e = funcArgTypes.size(); Idx != e; ++Idx, ++AI) {
		Exp_VarDef* pVarDef = dynamic_cast<Exp_VarDef*>(mExpressions[Idx]);
		assert(pVarDef);
		if (mArgments[Idx].isByRef) {
			// Create a reference variable
			llvm::Value* funcArg = funcGC_ctx->NewVariable(pVarDef, AI);
		}
		else {
			llvm::Value* funcArg = funcGC_ctx->NewVariable(pVarDef, NULL);
			// Store the input argument's value in the the local variables.
			CG_Context::sBuilder.CreateStore(AI, funcArg);
		}
	}

	// The last expression of the function domain should be the function body(which is a child domain)
	CodeDomain* pFuncBody = dynamic_cast<CodeDomain*>(mExpressions[mArgments.size()]);
	assert(pFuncBody);
	pFuncBody->GenerateCode(funcGC_ctx);

	return F;
}

llvm::Value* CodeDomain::GenerateCode(CG_Context* context) const
{
	CG_Context* domain_ctx = context->CreateChildContext(context->GetCurrentFunc());
	for (int i = 0; i < (int)mExpressions.size(); ++i) {
		mExpressions[i]->GenerateCode(domain_ctx);
	}
	return NULL; // the domain doesn't have the value to return
}

llvm::Value* Exp_FuncRet::GenerateCode(CG_Context* context) const
{
	if (mpRetValue) {
		llvm::Value* retVal = mpRetValue->GenerateCode(context);
		assert(retVal);
		return CG_Context::sBuilder.CreateRet(retVal);
	}
	else
		return NULL;
}

void Exp_DotOp::GenerateAssignCode(CG_Context* context, llvm::Value* pValue) const
{
	int vecElemIdx = -1;
	llvm::Value* pVarPtr = GetValuePtr(context, vecElemIdx);
	assert(pVarPtr);
	if (vecElemIdx == -1) {
		if (GetCachedTypeInfo().type == VarType::kBoolean) {
			llvm::Value* falseValue = Constant::getIntegerValue(SC_INT_TYPE, APInt(sizeof(Int)*8, (uint64_t)0));
			llvm::Value* trueValue = Constant::getIntegerValue(SC_INT_TYPE, APInt(sizeof(Int)*8, (uint64_t)1));
			llvm::Value* intValue = CG_Context::sBuilder.CreateSelect(pValue, trueValue, falseValue);
			CG_Context::sBuilder.CreateStore(intValue, pVarPtr);
		}
		else
			CG_Context::sBuilder.CreateStore(pValue, pVarPtr);
	}
	else {
		llvm::Value* idx = Constant::getIntegerValue(SC_INT_TYPE, APInt(sizeof(Int)*8, (uint64_t)vecElemIdx));
		llvm::Value* updatedValue = CG_Context::sBuilder.CreateInsertElement(CG_Context::sBuilder.CreateLoad(pVarPtr), pValue, idx);
		CG_Context::sBuilder.CreateStore(updatedValue, pVarPtr);
	}
}

llvm::Value* Exp_DotOp::GetValuePtr(CG_Context* context, int& vecElemIdx) const
{
	int parentElemIdx = -1;
	llvm::Value* parentPtr = mpExp->GetValuePtr(context, parentElemIdx);
	
	Exp_ValueEval::TypeInfo parentTypeInfo;
	parentTypeInfo = mpExp->GetCachedTypeInfo();
	const Exp_StructDef* pParentStructDef = parentTypeInfo.pStructDef;
	vecElemIdx = -1;
	if (parentPtr && parentElemIdx == -1) {
		
		int elemIdx = -1;
		if (pParentStructDef)
			elemIdx = pParentStructDef->GetElementIdxByName(mOpStr);

		if (elemIdx != -1) {
			// It's accessing structure member
			std::vector<llvm::Value*> indices(2);
			indices[0] = Constant::getIntegerValue(SC_INT_TYPE, APInt(sizeof(Int)*8, (uint64_t)0));
			indices[1] = Constant::getIntegerValue(SC_INT_TYPE, APInt(sizeof(Int)*8, (uint64_t)elemIdx));
			llvm::Value* structElemPtr = CG_Context::sBuilder.CreateGEP(parentPtr, indices);
			return structElemPtr;//CG_Context::sBuilder.CreateGEP(structElemPtr, indices[0]);
		}
		else {
			// it should access ONE specific element with swizzling operator
			int elemCnt = TypeElementCnt(GetCachedTypeInfo().type);
			if (elemCnt != 1)
				return NULL;
			else {
				int swizzleIdx[4];
				ConvertSwizzle(mOpStr.c_str(), swizzleIdx);
				vecElemIdx = swizzleIdx[0];
				return parentPtr;
			}
		}
	}
	else 
		return NULL;
	
}

llvm::Value* Exp_DotOp::GenerateCode(CG_Context* context) const
{
	int firstElemIdx = -1;
	llvm::Value* dataPtr = GetValuePtr(context, firstElemIdx);
	if (dataPtr) {
		
		llvm::Value* ret = CG_Context::sBuilder.CreateLoad(dataPtr);
		if  (firstElemIdx != -1) {
			llvm::Value* idx = Constant::getIntegerValue(SC_INT_TYPE, APInt(sizeof(Int)*8, (uint64_t)firstElemIdx));
			ret = CG_Context::sBuilder.CreateExtractElement(ret, idx);
		}
		return ret;
	}
	else {
		// It should be a vector swizzling
		int swizzleIdx[4];
		int elemCnt = ConvertSwizzle(mOpStr.c_str(), swizzleIdx);
		llvm::SmallVector<Constant*, 4> Idxs;
		for (int i = 0; i < elemCnt; ++i) 
			Idxs.push_back(CG_Context::sBuilder.getInt32(swizzleIdx[i]));
		llvm::Value* srcValue = mpExp->GenerateCode(context);
		llvm::Value* swizzledValue = CG_Context::sBuilder.CreateShuffleVector(srcValue, llvm::UndefValue::get(srcValue->getType()), llvm::ConstantVector::get(Idxs));
		return swizzledValue;
	}
}

llvm::Value* Exp_BuiltInInitializer::GenerateCode(CG_Context* context) const
{
	int elemCnt = TypeElementCnt(mType);

	if (elemCnt == 1) {
		llvm::Value* tmpVar = mpSubExprs[0]->GenerateCode(context);
		return context->CastValueType(tmpVar, mpSubExprs[0]->GetCachedTypeInfo().type, mType);
	}
	else {
		int elemIdx = 0;
		llvm::Value* outVar = llvm::UndefValue::get(CG_Context::ConvertToLLVMType(mType));
		for (int exp_i = 0; exp_i < 4; ++exp_i) {
			if (mpSubExprs[exp_i] == NULL)
				break;
			llvm::Value* tmpVar = mpSubExprs[exp_i]->GenerateCode(context);
			VarType subType = mpSubExprs[exp_i]->GetCachedTypeInfo().type;
			int subElemCnt = TypeElementCnt(subType);
			
			VarType destElemType = IsIntegerType(mType) ? VarType::kInt : VarType::kFloat;
			VarType srcElemType = IsIntegerType(subType) ? VarType::kInt : VarType::kFloat;
			if (subElemCnt > 1) {
				for (int i = 0; i < subElemCnt; ++i) {
					llvm::Value* idx = Constant::getIntegerValue(SC_INT_TYPE, APInt(sizeof(Int)*8, (uint64_t)i));
					llvm::Value* elemValue = CG_Context::sBuilder.CreateExtractElement(tmpVar, idx);
					elemValue = context->CastValueType(elemValue, srcElemType, destElemType);

					idx = Constant::getIntegerValue(SC_INT_TYPE, APInt(sizeof(Int)*8, (uint64_t)elemIdx++));
					outVar = CG_Context::sBuilder.CreateInsertElement(outVar, elemValue, idx);
				}
			}
			else {
				llvm::Value* idx = Constant::getIntegerValue(SC_INT_TYPE, APInt(sizeof(Int)*8, (uint64_t)elemIdx++));
				llvm::Value* elemValue = tmpVar;
				elemValue = context->CastValueType(elemValue, subType, destElemType);

				outVar = CG_Context::sBuilder.CreateInsertElement(outVar, elemValue, idx);
			}
		}
		
		return outVar;
	}
}

llvm::Value* Exp_VariableRef::GetValuePtr(CG_Context* context, int& vecElemIdx) const
{
	vecElemIdx = -1;
	llvm::Value* varPtr = context->GetVariablePtr(mpDef->GetVarName().ToStdString(), true);
	return varPtr;
}

llvm::Value* Exp_Indexer::GenerateCode(CG_Context* context) const
{
	int elemIdx = -1;
	llvm::Value* ptr = GetValuePtr(context, elemIdx);
	return CG_Context::sBuilder.CreateLoad(ptr);
}

llvm::Value* Exp_Indexer::GetValuePtr(CG_Context* context, int& vecElemIdx) const
{
	vecElemIdx = -1;
	llvm::Value* idx = mpIndex->GenerateCode(context);
	int elemIdx = -1;
	llvm::Value* parentPtr = mpExp->GetValuePtr(context, elemIdx);
	assert(elemIdx = -1);

	std::vector<llvm::Value*> indices(2);
	indices[1] = idx;
	indices[0] = Constant::getIntegerValue(SC_INT_TYPE, APInt(sizeof(Int)*8, (uint64_t)0));
	return CG_Context::sBuilder.CreateGEP(parentPtr, indices);
}

void Exp_Indexer::GenerateAssignCode(CG_Context* context, llvm::Value* pValue) const
{
	int elemIdx = -1;
	llvm::Value* ptr = GetValuePtr(context, elemIdx);
	CG_Context::sBuilder.CreateStore(pValue, ptr);
}

} // namespace SC