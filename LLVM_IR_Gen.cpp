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
	Function *mpCurFunction;
	std::hash_map<std::string, llvm::Value*> mVariables;
public:
	static llvm::Type* ConvertToLLVMType(VarType tp);

	llvm::Value* GetVariable(const std::string& name, bool includeParent);
	llvm::Value* NewVariable(const std::string& name, Exp_VarDef* pVarDef);
	llvm::Value* NewStructDef(const std::string& name, Exp_StructDef* pStructDef);
};

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

llvm::Value* CG_Context::NewVariable(const std::string& name, Exp_VarDef* pVarDef)
{
	assert(mpCurFunction);
	if (mVariables.find(name) != mVariables.end())
		return NULL;
	IRBuilder<> TmpB(&mpCurFunction->getEntryBlock(),
                 mpCurFunction->getEntryBlock().begin());
	llvm::Value* ret = NULL;
	if (pVarDef->GetVarType() == VarType::kStructure) {
		// TODO:
		return NULL;
	}
	else {
		llvm::Type* llvmType = CG_Context::ConvertToLLVMType(pVarDef->GetVarType());
		if (llvmType)
			ret = TmpB.CreateAlloca(llvmType, 0, name.c_str());
		return ret;
	}
}

llvm::Value* CG_Context::NewStructDef(const std::string& name, Exp_StructDef* pStructDef)
{
	// TODO:
	return NULL;
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
	return context->NewVariable(varName, this);
}

llvm::Value* Exp_StructDef::GenerateCode(CG_Context* context)
{
	return NULL;
}

} // namespace SC