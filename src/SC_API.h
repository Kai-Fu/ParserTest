#pragma once
#include <stdio.h>

#ifdef KSC_BUILD_LIB
#define KSC_API __declspec(dllexport)
#else
#define KSC_API
#endif

typedef void* ModuleHandle;
typedef void* StructHandle;
typedef void* FunctionHandle;
typedef void* (*PFN_TypeInitializer)(const char* pInitData);

namespace SC {
	typedef float Float;
	typedef int Int;
	typedef int Boolean;
	
	enum VarType {
		kFloat,
		kFloat2,
		kFloat3,
		kFloat4,

		kInt,
		kInt2,
		kInt3,
		kInt4,

		kBoolean,

		kStructure,
		kVoid,
		kExternType,
		kInvalid
	};

}

struct KSC_TypeInfo
{
	SC::VarType type;
	int arraySize;
	StructHandle hStruct;
};

extern "C" {

	KSC_API bool KSC_Initialize(const char* sharedCode = NULL);
	KSC_API const char* KSC_GetLastErrorMsg();
	KSC_API bool KSC_AddExternalFunction(const char* funcName, void* funcPtr);
	KSC_API void KSC_AddExternalTypeInitializer(const char* typeName, PFN_TypeInitializer initFunc);
	KSC_API bool KSC_Compile(const char* sourceCode, const char** funcNames, int funcCnt, void** funcPtr);

	KSC_API FunctionHandle KSC_GetFunctionHandleByName(const char* funcName, ModuleHandle hModule);
	KSC_API int KSC_GetFunctionArgumentCount(FunctionHandle hFunc, ModuleHandle hModule);
	KSC_API KSC_TypeInfo KSC_GetFunctionArgumentType(FunctionHandle hFunc, ModuleHandle hModule, int argIdx);
	KSC_API KSC_TypeInfo KSC_GetFunctionReturnType(FunctionHandle hFunc, ModuleHandle hModule);

	KSC_API StructHandle KSC_GetStructHandleByName(const char* structName, ModuleHandle hModule);
	KSC_API void* KSC_GetStructMember(StructHandle hStruct, ModuleHandle hModule, const char* member);
	KSC_API KSC_TypeInfo KSC_GetStructMemberType(StructHandle hStruct, ModuleHandle hModule, const char* member);
	KSC_API bool KSC_SetStructMember(StructHandle hStruct, ModuleHandle hModule, const char* member, void* data);
	KSC_API int KSC_GetStructSize(StructHandle hStruct, ModuleHandle hModule);


}

