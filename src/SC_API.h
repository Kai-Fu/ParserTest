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
	bool isRef;
	StructHandle hStruct;
};

extern "C" {

	KSC_API bool KSC_Initialize(const char* sharedCode = NULL);
	KSC_API void KSC_Destory();
	KSC_API const char* KSC_GetLastErrorMsg();
	KSC_API bool KSC_AddExternalFunction(const char* funcName, void* funcPtr);

	KSC_API ModuleHandle KSC_Compile(const char* sourceCode);
	KSC_API void* KSC_GetFunctionPtr(const char* funcName, ModuleHandle hModule);

	KSC_API FunctionHandle KSC_GetFunctionHandleByName(const char* funcName, ModuleHandle hModule);
	KSC_API int KSC_GetFunctionArgumentCount(FunctionHandle hFunc);
	KSC_API KSC_TypeInfo KSC_GetFunctionArgumentType(FunctionHandle hFunc, int argIdx);
	KSC_API KSC_TypeInfo KSC_GetFunctionReturnType(FunctionHandle hFunc);

	KSC_API StructHandle KSC_GetStructHandleByName(const char* structName, ModuleHandle hModule);
	KSC_API KSC_TypeInfo KSC_GetStructMemberType(StructHandle hStruct, const char* member);
	KSC_API void* KSC_GetStructMemberPtr(StructHandle hStruct, void* pStructVar, const char* member);
	KSC_API bool KSC_SetStructMemberData(StructHandle hStruct, void* pStructVar, const char* member, void* data);
	KSC_API int KSC_GetStructSize(StructHandle hStruct);


}

