// SC.cpp : Defines the entry point for the console application.
//

#include <stdio.h>
#include "SC_API.h"
#include <string.h>
#include <assert.h>

struct TestStructure
{
	float var0[4];  // float3
	int var1[2]; // int2
};

int main(int argc, char* argv[])
{
	KSC_Initialize();

	FILE* f = NULL;
	fopen_s(&f, "struct_mem_layout.ls", "r");
	if (f == NULL)
		return -1;
	fseek(f, 0, SEEK_END);
	long len = ftell(f);
	fseek(f, 0, SEEK_SET);

	char* content = new char[len + 1];
	char* line = content;
	size_t totalLen = 0;

	while (fgets(line, len, f) != NULL) {
		size_t lineLen = strlen(line);
		line += lineLen;
		totalLen += lineLen;
	}

	if (totalLen == 0)
		return -1;
	else {
		content[totalLen] = '\0';

		ModuleHandle hModule = KSC_Compile(content);
		if (!hModule) {
			printf(KSC_GetLastErrorMsg());
			return -1;
		}

		typedef int (*PFN_RW_Structure)(TestStructure* arg, void* arg1);
		
		TestStructure tempStruct;
		FunctionHandle hFunc = KSC_GetFunctionHandleByName("PFN_RW_Structure", hModule);
		assert(KSC_GetFunctionArgumentCount(hFunc) == 2);

		// The the type info of the second argument
		KSC_TypeInfo typeInfo = KSC_GetFunctionArgumentType(hFunc, 1);
		printf("Argument(1) type stirng is %s, size is %d.\n", typeInfo.typeString, KSC_GetStructSize(typeInfo.hStruct));
		assert(KSC_GetStructHandleByName(typeInfo.typeString, hModule));

		void* tempStruct1 = KSC_AllocMemForType(typeInfo, 1);
		float* pVar0 = (float*)KSC_GetStructMemberPtr(typeInfo.hStruct, tempStruct1, "var0");
		int* pVar1 = (int*)KSC_GetStructMemberPtr(typeInfo.hStruct, tempStruct1, "var1");
		pVar0[0] = 7.0f;
		pVar0[1] = 8.0f;
		pVar0[2] = 9.0f;
		pVar0[3] = 10.0f;
		pVar1[0] = 6;
		pVar1[1] = 7;

		PFN_RW_Structure RW_Structure = (PFN_RW_Structure)KSC_GetFunctionPtr(hFunc);
		int ret = RW_Structure(&tempStruct, tempStruct1);
		printf("Test finished!\n");
	}
	

	return 0;
}

