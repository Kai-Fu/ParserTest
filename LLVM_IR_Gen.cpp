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
	llvm::Value* GetVariable(const std::string& name, bool includeParent);
	llvm::Value* NewVariable(const std::string& name, VarType tp);
};

llvm::Value* CG_Context::GetVariable(const std::string& name, bool includeParent)
{
	std::hash_map<std::string, llvm::Value*>::iterator it = mVariables.find(name);
	if (it == mVariables.end() && includeParent)
		return mpParent ? mpParent->GetVariable(name, true) : NULL;
	else
		return it->second;
}

llvm::Value* CG_Context::NewVariable(const std::string& name, VarType tp)
{
	assert(mpCurFunction);
	if (mVariables.find(name) != mVariables.end())
		return NULL;
	IRBuilder<> TmpB(&mpCurFunction->getEntryBlock(),
                 mpCurFunction->getEntryBlock().begin());
	llvm::Value* ret = NULL;
	switch (tp) {
	case VarType::kFloat:
		ret = TmpB.CreateAlloca(SC_FLOAT_TYPE, 0, name.c_str());
		break;
	case VarType::kFloat2:
		ret = TmpB.CreateAlloca(VectorType::get(SC_FLOAT_TYPE, 2), 0, name.c_str());
		break;
	case VarType::kFloat3:
		ret = TmpB.CreateAlloca(VectorType::get(SC_FLOAT_TYPE, 3), 0, name.c_str());
		break;
	case VarType::kFloat4:
		ret = TmpB.CreateAlloca(VectorType::get(SC_FLOAT_TYPE, 4), 0, name.c_str());
		break;

	case VarType::kInt:
		ret = TmpB.CreateAlloca(SC_INT_TYPE, 0, name.c_str());
		break;
	case VarType::kInt2:
		ret = TmpB.CreateAlloca(VectorType::get(SC_INT_TYPE, 2), 0, name.c_str());
		break;
	case VarType::kInt3:
		ret = TmpB.CreateAlloca(VectorType::get(SC_INT_TYPE, 3), 0, name.c_str());
		break;
	case VarType::kInt4:
		ret = TmpB.CreateAlloca(VectorType::get(SC_INT_TYPE, 4), 0, name.c_str());
		break;
	case VarType::kBoolean:
		ret = TmpB.CreateAlloca(SC_INT_TYPE, 0, name.c_str()); // Use integer to represent boolean value
		break;
	}

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
	return context->NewVariable(varName, mVarType);
}

} // namespace SC