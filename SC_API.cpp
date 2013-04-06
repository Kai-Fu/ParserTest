#include "SC_API.h"
#include "IR_Gen_Context.h"
#include "parser_AST_Gen.h"
#include <string>


static std::string			s_lastErrMsg;

bool InitializeKSC()
{
	SC::Initialize_AST_Gen();
	return SC::InitializeCodeGen();
}


void DestoryKSC()
{
	SC::DestoryCodeGen();
	SC::Finish_AST_Gen();
}


const char* GetLastErrorMsg()
{
	return s_lastErrMsg.c_str();
}

bool AddExternalFunction(const char* functDecl, void* funcPtr)
{
	// TODO:
	return true;
}

bool Compile(const char* sourceCode, const char** funcNames, int funcCnt, void** funcPtr)
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