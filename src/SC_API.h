#pragma once

#ifdef KSC_BUILD_LIB
#define KSC_API __declspec(dllexport)
#else
#define KSC_API
#endif

extern "C" {

	KSC_API bool KSC_Initialize();
	KSC_API const char* KSC_GetLastErrorMsg();
	KSC_API bool KSC_AddExternalFunction(const char* funcName, void* funcPtr);
	KSC_API bool KSC_Compile(const char* sourceCode, const char** funcNames, int funcCnt, void** funcPtr);

}

