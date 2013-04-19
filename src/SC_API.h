#pragma once

#ifdef KSC_BUILD_LIB
#define KSC_API __declspec(dllexport)
#else
#define KSC_API
#endif

typedef void* ModuleHandle;
typedef void* StructHandle;
typedef void* (*PFN_TypeInitializer)(const char* pInitData);

extern "C" {

	KSC_API bool KSC_Initialize();
	KSC_API const char* KSC_GetLastErrorMsg();
	KSC_API bool KSC_AddExternalFunction(const char* funcName, void* funcPtr);
	KSC_API void KSC_AddExternalTypeInitializer(const char* typeName, PFN_TypeInitializer initFunc);
	KSC_API bool KSC_Compile(const char* sourceCode, const char** funcNames, int funcCnt, void** funcPtr);

	KSC_API StructHandle GetStructHandleByName(const char* structName, ModuleHandle hModule);
	KSC_API bool CreateStructData(StructHandle hStruct, ModuleHandle hModule, void* pData);
	KSC_API int GetStructDataSize(StructHandle hStruct, ModuleHandle hModule);


}

