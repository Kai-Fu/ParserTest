#pragma once

#ifdef KSC_BUILD_LIB
#define KSC_API __declspec(dllexport)
#else
#define KSC_API
#endif

extern "C" {

	KSC_API bool InitializeKSC();
	KSC_API void DestoryKSC();
	KSC_API const char* GetLastErrorMsg();
	KSC_API bool AddExternalFunction(const char* functDecl, void* funcPtr);
	KSC_API bool Compile(const char* sourceCode, const char** funcNames, int funcCnt, void** funcPtr);

}

