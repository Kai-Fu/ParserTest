#include "SC_API.h"
#include "IR_Gen_Context.h"
#include "parser_AST_Gen.h"
#include <string>
#include <list>

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

void KSC_AddExternalTypeInitializer(const char* typeName, PFN_TypeInitializer initFunc)
{
	SC::CG_Context::sExternalTypeInitializers[typeName] = initFunc;
}

bool KSC_Compile(const char* sourceCode, const char** funcNames, int funcCnt, void** funcPtr)
{
#ifdef WANT_MEM_LEAK_CHECK
	size_t expInstCnt = SC::Expression::s_instances.size();
#endif	

	bool ret = false;
	{
		KSC_ModuleDesc* pModuleDesc = new KSC_ModuleDesc;
		SC::CompilingContext scContext(NULL);
		std::auto_ptr<SC::RootDomain> scDomain(scContext.Parse(sourceCode, s_predefineDomain));
		if (scDomain.get() == NULL) {
			scContext.PrintErrorMessage(&s_lastErrMsg);
		}
		else {
			if (!scDomain->JIT_Compile(&s_predefineCtx, *pModuleDesc)) {
				delete pModuleDesc;
				s_lastErrMsg = "Failed to compile.";
			}
			else{
				s_modules.push_back(pModuleDesc);
				int i = 0;
				for (; i < funcCnt; ++i) {
					funcPtr[i] = scDomain->GetFuncPtrByName(funcNames[i]);
					if (funcPtr[i] == NULL)
						break;
				}
				ret = (i == funcCnt);
			}
		}
	}

#ifdef WANT_MEM_LEAK_CHECK
	assert(SC::Expression::s_instances.size() == expInstCnt);
#endif
	return ret;
}