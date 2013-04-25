// SC.cpp : Defines the entry point for the console application.
//

#include <stdio.h>
#include "SC_API.h"
#include <string.h>

float SimpleCallee(float* arg) 
{
	*arg = *arg + 123.5f;
	return *arg;
}

int main(int argc, char* argv[])
{
	KSC_Initialize();

	FILE* f = NULL;
	fopen_s(&f, "basic_expressions.ls", "r");
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

		typedef int (*PFN_TestBooleanValues)();
		FunctionHandle hFunc = KSC_GetFunctionHandleByName("TestBooleanValues", hModule);
		PFN_TestBooleanValues TestBooleanValues = (PFN_TestBooleanValues)KSC_GetFunctionPtr(hFunc);
		int result = TestBooleanValues();
		printf("result is %d\n", result);

		{
			typedef float (*PFN_DotProduct)(float* l, float* r);
			hFunc = KSC_GetFunctionHandleByName("DotProduct", hModule);
			PFN_DotProduct DotProduct = (PFN_DotProduct)KSC_GetFunctionPtr(hFunc);
			float ll[3] = {0.7f, 0.6f, 0.3f};
			float rr[3] = {0.2f, 0.8f, 0.13f};
			float fResult = DotProduct(ll, rr);
			printf("result is %f\n", fResult);
		}

		{
			typedef float (*PFN_TestSwizzle)(float* l);
			hFunc = KSC_GetFunctionHandleByName("TestSwizzle", hModule);
			PFN_TestSwizzle TestSwizzle = (PFN_TestSwizzle)KSC_GetFunctionPtr(hFunc);
			float ll[3] = {0.7f, 0.6f, 0.3f};
			float fResult = TestSwizzle(ll);
			printf("result is %f\n", fResult);
		}
	}
	

	return 0;
}

