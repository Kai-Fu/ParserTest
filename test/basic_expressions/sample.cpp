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
		const char* funcNames[3];
		void* funcPtrs[3];

		funcNames[0] = "TestBooleanValues";
		funcNames[1] = "DotProduct";
		funcNames[2] = "TestSwizzle";


		if (!KSC_Compile(content, funcNames, 3, funcPtrs)) {
			printf(KSC_GetLastErrorMsg());
			return -1;
		}

		typedef int (*PFN_TestBooleanValues)();
		PFN_TestBooleanValues TestBooleanValues = (PFN_TestBooleanValues)funcPtrs[0];
		int result = TestBooleanValues();
		printf("result is %d\n", result);

		{
			typedef float (*PFN_DotProduct)(float* l, float* r);
			PFN_DotProduct DotProduct = (PFN_DotProduct)funcPtrs[1];
			float ll[3] = {0.7f, 0.6f, 0.3f};
			float rr[3] = {0.2f, 0.8f, 0.13f};
			float fResult = DotProduct(ll, rr);
			printf("result is %f\n", fResult);
		}

		{
			typedef float (*PFN_TestSwizzle)(float* l);
			PFN_TestSwizzle TestSwizzle = (PFN_TestSwizzle)funcPtrs[2];
			float ll[3] = {0.7f, 0.6f, 0.3f};
			float fResult = TestSwizzle(ll);
			printf("result is %f\n", fResult);
		}
	}
	

	return 0;
}

