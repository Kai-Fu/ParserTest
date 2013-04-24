#pragma once
#include <stdio.h>

#ifdef KSC_BUILD_LIB
#define KSC_API __declspec(dllexport)
#else
#define KSC_API
#endif

/*!
	This file contains the APIs for KSC(Kai's Shader Compiler).
	The primary goal of KSC is to provide the compilation and JIT support for HLSL-like language, 
	so that the hosting C++ application can compile the HLSL-like code on the fly and JIT the requested
	functions.
	The HLSL-like language, which KSC supports, is called KSCL. Its language spec is described in
	another document, however here in short you can assume it is a superset of C language with HLSL swizzling support.
	Several of vector types are native built-in types, such as float4, float3 and int4. Differing from C,
	the pointer type is not supported for safety reason(KSCL is a "safe" language, see more for this in KSCL sepc). 
	Passing-by-reference is supported and the JIT-ed function will treat the referenced arguments as C++ pointers, 
	thus it gives the ablity to modify the data from C++ side in KSCL-implemented functions.
	The KSC APIs are kept as simple as possible, you may refer to each API for the details.
*/

/**
	The module handle is the representation for one compiling session. The compiling infomation are kept within 
	the domain of module so different modules cannot share any information. 
*/
typedef void* ModuleHandle;

/**
	The structure handle is the representation of one user-defined structure type in KSCL.
*/
typedef void* StructHandle;

/**
	The function handle is the representation of one user-defined function in KSCL.
*/
typedef void* FunctionHandle;

namespace SC {
	// The following are the single-value types that KSC support.
	typedef float Float;
	typedef int Int;
	typedef int Boolean;
	
	// This enum contains all the supported types including vector types.
	// Note external type(kExternType) is treated as void pointer.
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

// The type information retrieved KSC APIs.
struct KSC_TypeInfo
{
	SC::VarType type;
	int arraySize;
	bool isRef;
	StructHandle hStruct;
	const char* typeString;
};

extern "C" {

	/**
		The initialization function of KSC. It should be called before any other APIs get called.
		The argument "sharedCode" is the code that will be shared between multiple modules, e.g. some global
		functions or structure definitions. If the shared code contains bad syntax this function will fail.
	*/
	KSC_API bool KSC_Initialize(const char* sharedCode = NULL);

	/**
		The destroy function of KSC. It should be called when the client application is done for KSC,
		which means all the handles, type information as well as JIT-ed functions are invalid after
		the invoking of this function. For applications that initialize KSC only once, there's no need
		to call this function before exit since the resource is auto-cleaned when the application quits.
	*/
	KSC_API void KSC_Destory();

	/**
		If any KSC API fails for whatever reason including compiling error, call this function to retrieve
		the error message.
	*/
	KSC_API const char* KSC_GetLastErrorMsg();

	/**
		There're cases that the client application wants its KSC code to be able to invoke a function that is
		implemented in hosting C++ code. Use this function to tell KSC the function pointer and its function name
		that will link with KSCL.
		In KSCL side, you still need to declare the function without body implementation in order to let KSC know
		it should look up in the external symbol for the implementation of this function.
	*/
	KSC_API bool KSC_AddExternalFunction(const char* funcName, void* funcPtr);

	/**
		This function compiles the KSCL code, it will return the module handle on succeed otherwise return NULL.
	*/
	KSC_API ModuleHandle KSC_Compile(const char* sourceCode);

	/**
		This funtion is to JIT the function with the name specified.
	*/
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

