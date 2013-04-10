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
		llvm::Value* initValue = context->CastValueType(mpInitValue->GenerateCode(context), mpInitValue->GetCachedTypeInfo().type, mVarType);
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
	Function *F = context->GetFuncDeclByName(mFuncName);
	llvm::Type* retType = NULL;
	if (!F) {
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
		if (mReturnType == VarType::kStructure) {
			retType = context->GetStructType(mpRetStruct);
		}
		else 
			retType = context->ConvertToLLVMType(mReturnType);

		FunctionType *FT = FunctionType::get(retType, funcArgTypes, false);
		F = Function::Create(FT, Function::ExternalLinkage, mFuncName, CG_Context::TheModule);
	}

	if (F) {
		context->AddFunctionDecl(mFuncName, F);
	}
	else {
		return NULL;
	}

	if (!mHasBody) {
		// Function doens't have the body, so it must be an external function.
		if (CG_Context::sGlobalFuncSymbols.find(mFuncName) != CG_Context::sGlobalFuncSymbols.end()) {
			CG_Context::TheExecutionEngine->addGlobalMapping(F, CG_Context::sGlobalFuncSymbols[mFuncName]);
			return F;
		}
		else {
			return NULL;
		}
	}

	// set names for all arguments
	{
		unsigned Idx = 0;
		for (Function::arg_iterator AI = F->arg_begin(); Idx != mArgments.size(); ++AI, ++Idx) 
			AI->setName(mArgments[Idx].token.ToStdString());
	}
	
	// Create a new basic block to start insertion into, this basic blokc is a must for a function.
	BasicBlock *BB = BasicBlock::Create(getGlobalContext(), mFuncName + "_entry", F);

	// Create the basic block for exiting code which handles the return value, it will be inserted into function body later.
	BasicBlock *retBB = BasicBlock::Create(getGlobalContext(), mFuncName + "_exit");
	
	CG_Context::sBuilder.SetInsertPoint(BB);
	llvm::Value* pRetValuePtr = CG_Context::sBuilder.CreateAlloca(retType, 0, mFuncName + "_retValue");
	CG_Context* funcGC_ctx = context->CreateChildContext(F, retBB, pRetValuePtr);

	Function::arg_iterator AI = F->arg_begin();
	for (int Idx = 0, e = mArgments.size(); Idx != e; ++Idx, ++AI) {
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

	// Now insert the exit basic block
	F->getBasicBlockList().push_back(retBB);
	CG_Context::sBuilder.CreateBr(retBB);

	assert(retType);
	CG_Context::sBuilder.SetInsertPoint(retBB);
	if (mReturnType == VarType::kVoid)
		return CG_Context::sBuilder.CreateRetVoid();
	else
		CG_Context::sBuilder.CreateRet(CG_Context::sBuilder.CreateLoad(pRetValuePtr));


	delete funcGC_ctx;
	return F;
}

llvm::Value* CodeDomain::GenerateCode(CG_Context* context) const
{
	CG_Context* domain_ctx = context->CreateChildContext(context->GetCurrentFunc(), context->GetFuncRetBlk(), context->GetRetValuePtr());
	for (int i = 0; i < (int)mExpressions.size(); ++i) {
		mExpressions[i]->GenerateCode(domain_ctx);
	}
	delete domain_ctx;
	return NULL; // the domain doesn't have the value to return
}

llvm::Value* Exp_FuncRet::GenerateCode(CG_Context* context) const
{
	if (mpRetValue) {
		llvm::Value* retVal = mpRetValue->GenerateCode(context);
		assert(retVal);
		return CG_Context::sBuilder.CreateStore(retVal, context->GetRetValuePtr());
	}
	return NULL;
}

void Exp_DotOp::GenerateAssignCode(CG_Context* context, llvm::Value* pValue) const
{
	Exp_ValueEval::ValuePtrInfo valuePtrInfo = GetValuePtr(context);
	assert(valuePtrInfo.valuePtr);
	if (!valuePtrInfo.belongToVector) {
		if (GetCachedTypeInfo().type == VarType::kBoolean) {
			llvm::Value* falseValue = Constant::getIntegerValue(SC_INT_TYPE, APInt(sizeof(Int)*8, (uint64_t)0));
			llvm::Value* trueValue = Constant::getIntegerValue(SC_INT_TYPE, APInt(sizeof(Int)*8, (uint64_t)1));
			llvm::Value* intValue = CG_Context::sBuilder.CreateSelect(pValue, trueValue, falseValue);
			CG_Context::sBuilder.CreateStore(intValue, valuePtrInfo.valuePtr);
		}
		else
			CG_Context::sBuilder.CreateStore(pValue, valuePtrInfo.valuePtr);
	}
	else {
		llvm::Value* idx = Constant::getIntegerValue(SC_INT_TYPE, APInt(sizeof(Int)*8, (uint64_t)valuePtrInfo.vecElemIdx));
		llvm::Value* updatedValue = CG_Context::sBuilder.CreateInsertElement(CG_Context::sBuilder.CreateLoad(valuePtrInfo.valuePtr), pValue, idx);
		CG_Context::sBuilder.CreateStore(updatedValue, valuePtrInfo.valuePtr);
	}
}

Exp_ValueEval::ValuePtrInfo Exp_DotOp::GetValuePtr(CG_Context* context) const
{
	Exp_ValueEval::ValuePtrInfo parentPtrInfo = mpExp->GetValuePtr(context);
	
	Exp_ValueEval::TypeInfo parentTypeInfo;
	parentTypeInfo = mpExp->GetCachedTypeInfo();
	const Exp_StructDef* pParentStructDef = parentTypeInfo.pStructDef;
	Exp_ValueEval::ValuePtrInfo retValuePtr;
	retValuePtr.vecElemIdx = -1;
	retValuePtr.valuePtr = NULL;
	retValuePtr.belongToVector = false;

	if (parentPtrInfo.valuePtr != NULL && parentPtrInfo.vecElemIdx == -1) {
		
		int elemIdx = -1;
		if (pParentStructDef)
			elemIdx = pParentStructDef->GetElementIdxByName(mOpStr);

		if (elemIdx != -1) {
			// It's accessing structure member
			std::vector<llvm::Value*> indices(2);
			indices[0] = Constant::getIntegerValue(SC_INT_TYPE, APInt(sizeof(Int)*8, (uint64_t)0));
			indices[1] = Constant::getIntegerValue(SC_INT_TYPE, APInt(sizeof(Int)*8, (uint64_t)elemIdx));
			llvm::Value* structElemPtr = CG_Context::sBuilder.CreateGEP(parentPtrInfo.valuePtr, indices);
			retValuePtr.valuePtr = structElemPtr;
			retValuePtr.belongToVector = false;
			return retValuePtr;
		}
		else {
			// it should access ONE specific element with swizzling operator
			int elemCnt = TypeElementCnt(GetCachedTypeInfo().type);
			if (elemCnt != 1) {
				return retValuePtr;
			}
			else {
				int swizzleIdx[4];
				ConvertSwizzle(mOpStr.c_str(), swizzleIdx);
				retValuePtr.valuePtr = parentPtrInfo.valuePtr;
				retValuePtr.belongToVector = true;
				retValuePtr.vecElemIdx = swizzleIdx[0];
				return retValuePtr;
			}
		}
	}
	else 
		return retValuePtr;
	
}

llvm::Value* Exp_DotOp::GenerateCode(CG_Context* context) const
{
	Exp_ValueEval::ValuePtrInfo valuePtrInfo = GetValuePtr(context);
	if (valuePtrInfo.valuePtr) {
		
		llvm::Value* ret = CG_Context::sBuilder.CreateLoad(valuePtrInfo.valuePtr);
		if  (valuePtrInfo.belongToVector) {
			llvm::Value* idx = Constant::getIntegerValue(SC_INT_TYPE, APInt(sizeof(Int)*8, (uint64_t)valuePtrInfo.vecElemIdx));
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

Exp_ValueEval::ValuePtrInfo Exp_VariableRef::GetValuePtr(CG_Context* context) const
{
	Exp_ValueEval::ValuePtrInfo retValuePtr;
	retValuePtr.belongToVector = false;
	retValuePtr.valuePtr = context->GetVariablePtr(mpDef->GetVarName().ToStdString(), true);
	retValuePtr.vecElemIdx = -1;
	return retValuePtr;
}

llvm::Value* Exp_Indexer::GenerateCode(CG_Context* context) const
{
	Exp_ValueEval::ValuePtrInfo ptrInfo = GetValuePtr(context);
	assert(ptrInfo.belongToVector == false);
	return CG_Context::sBuilder.CreateLoad(ptrInfo.valuePtr);
}

Exp_ValueEval::ValuePtrInfo Exp_Indexer::GetValuePtr(CG_Context* context) const
{
	Exp_ValueEval::ValuePtrInfo retValuePtr;
	retValuePtr.vecElemIdx = -1;
	retValuePtr.belongToVector = false;
	llvm::Value* idx = mpIndex->GenerateCode(context);
	Exp_ValueEval::ValuePtrInfo parentPtrInfo = mpExp->GetValuePtr(context);
	assert(parentPtrInfo.valuePtr != NULL && parentPtrInfo.belongToVector == false);

	std::vector<llvm::Value*> indices(2);
	indices[1] = idx;
	indices[0] = Constant::getIntegerValue(SC_INT_TYPE, APInt(sizeof(Int)*8, (uint64_t)0));
	retValuePtr.valuePtr = CG_Context::sBuilder.CreateGEP(parentPtrInfo.valuePtr, indices);
	return retValuePtr;
}

void Exp_Indexer::GenerateAssignCode(CG_Context* context, llvm::Value* pValue) const
{
	Exp_ValueEval::ValuePtrInfo ptrInfo = GetValuePtr(context);
	assert(ptrInfo.belongToVector == false);
	CG_Context::sBuilder.CreateStore(pValue, ptrInfo.valuePtr);
}

llvm::Value* Exp_FunctionCall::GenerateCode(CG_Context* context) const
{
	llvm::Function* pF = context->GetFuncDeclByName(mpFuncDef->GetFunctionName());
	assert(pF);
	std::vector<llvm::Value*> args;
	for (int i = 0; i < (int)mInputArgs.size(); ++i) {
		if (mpFuncDef->GetArgumentDesc(i)->isByRef) {
			Exp_ValueEval::ValuePtrInfo argPtrInfo = mInputArgs[i]->GetValuePtr(context);
			assert(argPtrInfo.valuePtr != NULL && argPtrInfo.belongToVector == false);
			args.push_back(argPtrInfo.valuePtr);
		}
		else {
			llvm::Value* argValue = mInputArgs[i]->GenerateCode(context);
			args.push_back(argValue);
		}
	}
	return CG_Context::sBuilder.CreateCall(pF, args);
}


llvm::Value* Exp_If::GenerateCode(CG_Context* context) const
{
	llvm::Value* condValue = mpCondValue->GenerateCode(context);
  
	Function *pCurFunc = context->GetCurrentFunc();
	assert(pCurFunc);

	// Create blocks for the then and else cases.  Insert the 'then' block at the
	// end of the function.
	BasicBlock* pThenBB = BasicBlock::Create(getGlobalContext(), "then", pCurFunc);
	BasicBlock* pElseBB = BasicBlock::Create(getGlobalContext(), "else");
	BasicBlock* pMergeBB = BasicBlock::Create(getGlobalContext(), "ifcont");
  
	CG_Context::sBuilder.CreateCondBr(condValue, pThenBB, pElseBB);
  
	// Emit then value.
	CG_Context::sBuilder.SetInsertPoint(pThenBB);
  
	if (mpIfDomain) {
		// Code gen for if block
		CG_Context* childCtx = context->CreateChildContext(pCurFunc, context->GetFuncRetBlk(), context->GetRetValuePtr());
		mpIfDomain->GenerateCode(childCtx);
		delete childCtx;
	}
  
	CG_Context::sBuilder.CreateBr(pMergeBB);
	// Codegen of 'Then' can change the current block, update ThenBB for the PHI.
	pThenBB = CG_Context::sBuilder.GetInsertBlock();
  
	// Emit else block.
	pCurFunc->getBasicBlockList().push_back(pElseBB);
	CG_Context::sBuilder.SetInsertPoint(pElseBB);
  
	if (mpElseDomain) {
		// Code gen for else block
		CG_Context* childCtx = context->CreateChildContext(pCurFunc, context->GetFuncRetBlk(), context->GetRetValuePtr());
		mpElseDomain->GenerateCode(childCtx);
		delete childCtx;
	}
  
	CG_Context::sBuilder.CreateBr(pMergeBB);
	// Codegen of 'Else' can change the current block, update ElseBB for the PHI.
	pElseBB = CG_Context::sBuilder.GetInsertBlock();
  
	// Emit merge block.
	pCurFunc->getBasicBlockList().push_back(pMergeBB);
	CG_Context::sBuilder.SetInsertPoint(pMergeBB);
	llvm::Type* phiRetTy = SC_INT_TYPE;
	llvm::PHINode *PN = CG_Context::sBuilder.CreatePHI(phiRetTy, 2, "iftmp");
  
	llvm::Value* voidUndef = llvm::UndefValue::get(phiRetTy);
	PN->addIncoming(voidUndef, pThenBB);
	PN->addIncoming(voidUndef, pElseBB);
	return NULL;
}

llvm::Value* Exp_For::GenerateCode(CG_Context* context) const
{
	llvm::Type* phiRetTy = SC_INT_TYPE;
	llvm::Value* voidUndef = llvm::UndefValue::get(phiRetTy);

	CG_Context* pForCtx = context->CreateChildContext(context->GetCurrentFunc(), context->GetFuncRetBlk(), context->GetRetValuePtr());
	mStartStepCond->GetExpression(0)->GenerateCode(pForCtx);
	// Make the new basic block for the loop header, inserting after current block.
	llvm::Function* pCurFunc = pForCtx->GetCurrentFunc();
	llvm::BasicBlock *PreheaderBB = CG_Context::sBuilder.GetInsertBlock();
	llvm::BasicBlock *LoopBB = llvm::BasicBlock::Create(getGlobalContext(), "loop", pCurFunc);
  
	// Insert an explicit fall through from the current block to the LoopBB.
	CG_Context::sBuilder.CreateBr(LoopBB);

	// Start insertion in LoopBB.
	CG_Context::sBuilder.SetInsertPoint(LoopBB);
  
	// Start the PHI node with an entry for Start.
	PHINode *PN = CG_Context::sBuilder.CreatePHI(phiRetTy, 2);
	PN->addIncoming(voidUndef, PreheaderBB);
  
	// Emit the body of the loop.  This, like any other expr, can change the
	// current BB.  Note that we ignore the value computed by the body, but don't
	// allow an error.
	mForBody->GenerateCode(pForCtx);
  
	// Emit the step.
	mStartStepCond->GetExpression(2)->GenerateCode(pForCtx);
  
	// Compute the end condition.
	Value *contCond = mStartStepCond->GetExpression(1)->GenerateCode(pForCtx);
	assert(contCond);
  
	// Create the "after loop" block and insert it.
	BasicBlock *LoopEndBB = CG_Context::sBuilder.GetInsertBlock();
	BasicBlock *AfterBB = BasicBlock::Create(getGlobalContext(), "afterloop", pCurFunc);
  
	// Insert the conditional branch into the end of LoopEndBB.
	CG_Context::sBuilder.CreateCondBr(contCond, LoopBB, AfterBB);
  
	// Any new code will be inserted in AfterBB.
	CG_Context::sBuilder.SetInsertPoint(AfterBB);
  
	// Add a new entry to the PHI node for the backedge.
	PN->addIncoming(voidUndef, LoopEndBB);

	return NULL;
}

} // namespace SC