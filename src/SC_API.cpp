#include "SC_API.h"
#include "IR_Gen_Context.h"
#include "parser_AST_Gen.h"
#include <string>
#include <list>
#include <stdio.h>


static std::string			s_lastErrMsg;
SC::RootDomain*				s_predefineDomain = NULL;
SC::CG_Context				s_predefineCtx;
std::list<KSC_ModuleDesc*>	s_modules;

int __int_pow(int base, int p)
{
	return _Pow_int(base, p);
}

bool KSC_Initialize(const char* sharedCode)
{
	SC::Initialize_AST_Gen();
	bool ret = SC::InitializeCodeGen();
	
	if (ret) {
		SC::CompilingContext preContext(NULL);
		const char* intrinsicFuncDecal = 
			"float sin(float arg);\n"
			"float cos(float arg);\n"
			"float pow(float base, float exp);\n"
			"int ipow(int base, int exp);\n"
			"float sqrt(float arg);\n"
			"float fabs(float arg);\n";

		KSC_AddExternalFunction("sin", sinf);
		KSC_AddExternalFunction("cos", cosf);
		KSC_AddExternalFunction("pow", powf);
		KSC_AddExternalFunction("ipow", __int_pow);
		KSC_AddExternalFunction("sqrt", sqrtf);
		KSC_AddExternalFunction("fabs", fabsf);

		s_predefineDomain = new SC::RootDomain(NULL);
		if (!preContext.ParsePartial(intrinsicFuncDecal, s_predefineDomain))
			return false;

		if (sharedCode) {
			if (!preContext.ParsePartial(sharedCode, s_predefineDomain))
				return false;
		}

		for (int i = 0; i < s_predefineDomain->GetExpressionCnt(); ++i)
			s_predefineDomain->GetExpression(i)->GenerateCode(&s_predefineCtx);

		return true;
	}
	else
		return false;
}


void KSC_Destory()
{
	std::list<KSC_ModuleDesc*>::iterator it = s_modules.begin();
	for (; it != s_modules.end(); ++it) {
		delete *it;
	}
	SC::DestoryCodeGen();
	SC::Finish_AST_Gen();
}


const char* KSC_GetLastErrorMsg()
{
	return s_lastErrMsg.c_str();
}

bool KSC_AddExternalFunction(const char* funcName, void* funcPtr)
{
	SC::CG_Context::sGlobalFuncSymbols[funcName] = funcPtr;
	return true;
}

ModuleHandle KSC_Compile(const char* sourceCode)
{
#ifdef WANT_MEM_LEAK_CHECK
	size_t expInstCnt = SC::Expression::s_instances.size();
#endif	

	KSC_ModuleDesc* ret = NULL;
	{
		KSC_ModuleDesc* pModuleDesc = new KSC_ModuleDesc;
		SC::CompilingContext scContext(NULL);
		std::auto_ptr<SC::RootDomain> scDomain(scContext.Parse(sourceCode, s_predefineDomain));
		if (scDomain.get() == NULL) {
			scContext.PrintErrorMessage(&s_lastErrMsg);
		}
		else {
			if (!scDomain->CompileToIR(&s_predefineCtx, *pModuleDesc)) {
				delete pModuleDesc;
				s_lastErrMsg = "Failed to compile.";
			}
			else{
				s_modules.push_back(pModuleDesc);
				ret = pModuleDesc;
			}
		}
	}

#ifdef WANT_MEM_LEAK_CHECK
	assert(SC::Expression::s_instances.size() == expInstCnt);
#endif
	return ret;
}

void* KSC_GetFunctionPtr(FunctionHandle hFunc)
{
	KSC_FunctionDesc* pFuncDesc = (KSC_FunctionDesc*)hFunc;
	if (!pFuncDesc || !pFuncDesc->F)
		return NULL;

	llvm::Function* wrapperF = SC::CG_Context::CreateFunctionWithPackedArguments(*pFuncDesc);

	// The JIT-ed function should NOT have any optimized alignment, so here force it aligned to 1 byte.
	std::vector<Attributes::AttrVal> attrEnumList;
	attrEnumList.push_back(Attributes::Alignment);

	std::vector<llvm::AttributeWithIndex> attrWithIdxList;
	llvm::AttrBuilder attrBuilder;
	attrBuilder = attrBuilder.addAlignmentAttr(1);
	int argIdx = 0;
	for (Function::arg_iterator AI = wrapperF->arg_begin(); AI != wrapperF->arg_end(); ++AI, ++argIdx) {
		llvm::AttributeWithIndex attrWithIdx;
		attrWithIdx.Attrs = llvm::Attributes::get(getGlobalContext(), attrBuilder);
		attrWithIdx.Index = argIdx + 1;
		attrWithIdxList.push_back(attrWithIdx);
	}

	llvm::AttrListPtr attrList = llvm::AttrListPtr::get(getGlobalContext(), attrWithIdxList);
	wrapperF->setAttributes(attrList);
	wrapperF->dump();

	if (!llvm::verifyFunction(*wrapperF, llvm::PrintMessageAction))
		return SC::CG_Context::TheExecutionEngine->getPointerToFunction(wrapperF);
	else
		return NULL;
}

FunctionHandle KSC_GetFunctionHandleByName(const char* funcName, ModuleHandle hModule)
{
	KSC_ModuleDesc* pModule = (KSC_ModuleDesc*)hModule;
	if (!pModule)
		return NULL;

	if (pModule->mFunctionDesc.find(funcName) != pModule->mFunctionDesc.end()) {
		return pModule->mFunctionDesc[funcName];
	}
	else
		return NULL;
}

int KSC_GetFunctionArgumentCount(FunctionHandle hFunc)
{
	KSC_FunctionDesc* pFuncDesc = (KSC_FunctionDesc*)hFunc;
	if (!pFuncDesc)
		return NULL;

	return (int)pFuncDesc->mArgumentTypes.size();
}

KSC_TypeInfo KSC_GetFunctionArgumentType(FunctionHandle hFunc, int argIdx)
{
	KSC_TypeInfo ret = {SC::VarType::kInvalid, 0, 0, 0, NULL, NULL, false, false};
	KSC_FunctionDesc* pFuncDesc = (KSC_FunctionDesc*)hFunc;
	if (!pFuncDesc)
		return ret;

	return pFuncDesc->mArgumentTypes[argIdx];
}


StructHandle KSC_GetStructHandleByName(const char* structName, ModuleHandle hModule)
{
	KSC_ModuleDesc* pModule = (KSC_ModuleDesc*)hModule;
	if (!pModule)
		return NULL;

	if (pModule->mGlobalStructures.find(structName) != pModule->mGlobalStructures.end())
		return pModule->mGlobalStructures[structName];
	else
		return NULL;
}

KSC_TypeInfo KSC_GetStructMemberType(StructHandle hStruct, const char* member)
{
	KSC_TypeInfo ret = {SC::VarType::kInvalid, 0, 0, 0, NULL, NULL, false, false};
	KSC_StructDesc* pStructDesc = (KSC_StructDesc*)hStruct;
	if (!pStructDesc)
		return ret;
	std::hash_map<std::string, KSC_StructDesc::MemberInfo>::iterator it = pStructDesc->mMemberIndices.find(member);
	if (it != pStructDesc->mMemberIndices.end()) {
		return (*pStructDesc)[it->second.idx];
	}
	else
		return ret;
}

void* KSC_GetStructMemberPtr(StructHandle hStruct, void* pStructVar, const char* member)
{
	int offset = 0;
	KSC_StructDesc* pStructDesc = (KSC_StructDesc*)hStruct;
	if (!pStructDesc)
		return NULL;
	std::hash_map<std::string, KSC_StructDesc::MemberInfo>::iterator it = pStructDesc->mMemberIndices.find(member);
	if (it != pStructDesc->mMemberIndices.end()) {
		offset = it->second.mem_offset;
	}
	else
		return NULL;

	return ((unsigned char*)pStructVar + offset);
}

bool KSC_SetStructMemberData(StructHandle hStruct, void* pStructVar, const char* member, void* data)
{
	int offset = 0;
	KSC_StructDesc* pStructDesc = (KSC_StructDesc*)hStruct;
	if (!pStructDesc)
		return false;
	std::hash_map<std::string, KSC_StructDesc::MemberInfo>::iterator it = pStructDesc->mMemberIndices.find(member);
	if (it != pStructDesc->mMemberIndices.end()) {
		offset = it->second.mem_offset;
	}
	else
		return false;

	memcpy(((unsigned char*)pStructVar + offset), data, it->second.mem_size);
	return true;
}

int KSC_GetStructSize(StructHandle hStruct)
{
	int offset = 0;
	KSC_StructDesc* pStructDesc = (KSC_StructDesc*)hStruct;
	if (!pStructDesc)
		return 0;
	return pStructDesc->mStructSize;
}

void* _Aligned_Malloc(size_t size, size_t align)
{
#ifdef __GNUC__
	return posix_memalign(NULL, align, size);
#else
	return _aligned_malloc(size, align);
#endif
}

void _Aligned_Free(void* ptr)
{
#ifdef __GNUC__
	return free(ptr);
#else
	return _aligned_free(ptr);
#endif

}

void* KSC_AllocMemForType(const KSC_TypeInfo& typeInfo, int arraySize)
{
	return _Aligned_Malloc(typeInfo.sizeOfType * arraySize, typeInfo.alignment);
}

void KSC_FreeMem(void* pData)
{
	_Aligned_Free(pData);
}