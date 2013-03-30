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
		return Constant::getIntegerValue(SC_INT_TYPE, APInt(sizeof(Int)*8, (uint64_t)mValue, true));
}

llvm::Value* Exp_VarDef::GenerateCode(CG_Context* context)
{
	std::string varName = mVarName.ToStdString();
	llvm::Value* varPtr = context->NewVariable(this, NULL);
	if (mpInitValue) {
		llvm::Value* initValue = mpInitValue->GenerateCode(context);
		CG_Context::sBuilder.CreateStore(initValue, varPtr);
	}
	return varPtr;
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
	return context->GetVariableValue(mVariable.ToStdString(), true);
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


llvm::Value* Exp_VariableRef::GetValuePtr(CG_Context* context)
{
	llvm::Value* varPtr = context->GetVariablePtr(mpDef->GetVarName().ToStdString(), true);
	return varPtr;
}

llvm::Value* Exp_BinaryOp::GenerateCode(CG_Context* context)
{
	llvm::Value* VL = mpLeftExp->GenerateCode(context);
	llvm::Value* VR = mpRightExp->GenerateCode(context);
	if (!VL || !VR)
		return NULL;
	if (mOperator == "=") {
		llvm::Value* ptr = mpLeftExp->GetValuePtr(context);
		CG_Context::sBuilder.CreateStore(VR, ptr);
		return CG_Context::sBuilder.CreateLoad(ptr);
	}
	else {
		Exp_ValueEval::TypeInfo LtypeInfo, RtypeInfo;
		std::string errMsg;
		std::vector<std::string> warnMsg;
		mpLeftExp->CheckSemantic(LtypeInfo, errMsg, warnMsg);
		mpRightExp->CheckSemantic(RtypeInfo, errMsg, warnMsg);
		assert(!LtypeInfo.pStructDef);
		return context->CreateBinaryExpression(mOperator, VL, VR, 
			IsFloatType(LtypeInfo.type), TypeElementCnt(LtypeInfo.type), 
			IsFloatType(RtypeInfo.type), TypeElementCnt(RtypeInfo.type));
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

llvm::Value* Exp_DotOp::GetValuePtr(CG_Context* context)
{
	Exp_VariableRef* pVarRef = dynamic_cast<Exp_VariableRef*>(mpExp);
	std::string errMsg;
	std::vector<std::string> warnMsg;
	Exp_ValueEval::TypeInfo typeInfo;
	mpExp->CheckSemantic(typeInfo, errMsg, warnMsg);
	const Exp_StructDef* pStructDef = typeInfo.pStructDef;
	llvm::Value* dataPtr = NULL;
	if (pVarRef && typeInfo.pStructDef) {
		// This expression is only assignable when it is accessing the structure's member.
		std::string& refName = pVarRef->GetVarDef()->GetVarName().ToStdString();
		dataPtr = context->GetVariablePtr(refName, true);
	}
	else {
		Exp_DotOp* pDotOp = dynamic_cast<Exp_DotOp*>(mpExp);
		if (pDotOp) {
			dataPtr = pDotOp->GetValuePtr(context);
		}
		else
			return NULL;
	}

	std::vector<llvm::Value*> indices(2);
	indices[0] = Constant::getIntegerValue(SC_INT_TYPE, APInt(sizeof(Int)*8, (uint64_t)0));
	indices[1] = Constant::getIntegerValue(SC_INT_TYPE, APInt(sizeof(Int)*8, (uint64_t)pStructDef->GetElementIdxByName(mOpStr)));
	return CG_Context::sBuilder.CreateGEP(dataPtr, indices);
}

llvm::Value* Exp_DotOp::GenerateCode(CG_Context* context)
{
	llvm::Value* dataPtr = GetValuePtr(context);
	if (dataPtr) {
		return CG_Context::sBuilder.CreateLoad(dataPtr);
	}
	else 
		return NULL;
}

llvm::Value* Exp_BuiltInInitializer::GenerateCode(CG_Context* context)
{
	int elemCnt = TypeElementCnt(mType);
	if (elemCnt == 1)
		return mpSubExprs[0]->GenerateCode(context);
	else {
		llvm::Value* tmpVar = llvm::UndefValue::get(CG_Context::ConvertToLLVMType(mType));
		for (int i = 0; i < elemCnt; ++i) {
			llvm::Value* idx = Constant::getIntegerValue(SC_INT_TYPE, APInt(sizeof(Int)*8, (uint64_t)i));
			llvm::Value* elemValue = mpSubExprs[i]->GenerateCode(context);
			TypeInfo elemType;
			mpSubExprs[i]->CheckSemantic(elemType);
			if (IsFloatType(mType) && IsIntegerType(elemType.type))
				elemValue = CG_Context::sBuilder.CreateSIToFP(elemValue, SC_FLOAT_TYPE);
			else if (IsIntegerType(mType) && IsFloatType(elemType.type))
				elemValue = CG_Context::sBuilder.CreateFPToSI(elemValue, SC_INT_TYPE);
			tmpVar = CG_Context::sBuilder.CreateInsertElement(tmpVar, elemValue, idx);
		}
		return tmpVar;
	}
}

} // namespace SC