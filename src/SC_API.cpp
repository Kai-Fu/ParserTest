#include "SC_API.h"
#include "IR_Gen_Context.h"
#include "parser_AST_Gen.h"
#include <string>


static std::string			s_lastErrMsg;

bool KSC_Initialize()
{
	SC::Initialize_AST_Gen();
	bool ret = SC::InitializeCodeGen();
	
	const char* intrinsicFuncDecal = 
		"float sin(float arg);\n"
		"float cos(float arg);\n"
		"float pow(float base, float exp);\n"
		"int ipow(int base, int exp);\n"
		"float sqrt(float arg);\n"
		"float fabs(float arg);\n";

	if (ret) {
		KSC_Compile(intrinsicFuncDecal, NULL, 0, NULL);
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

bool KSC_Compile(const char* sourceCode, const char** funcNames, int funcCnt, void** funcPtr)
{
	SC::CompilingContext scContext(NULL);
	if (!scContext.Parse(sourceCode)) {
		scContext.PrintErrorMessage(&s_lastErrMsg);
		return false;
	}
	if (!scContext.JIT_Compile()) {
		s_lastErrMsg = "Failed to compile.";
		return false;
	}

	for (int i = 0; i < funcCnt; ++i) {
		funcPtr[i] = scContext.GetJITedFuncPtr(funcNames[i]);
	}
	return true;
}