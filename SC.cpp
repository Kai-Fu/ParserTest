// SC.cpp : Defines the entry point for the console application.
//

#include <stdio.h>
#include "SC_API.h"
#include <string.h>

struct TestStruct1
{
	float a;
	int cond;
	int ib[2];
	float b;
};

float SimpleCallee(float* arg) 
{
	*arg = *arg + 123.5f;
	return *arg;
}

int main(int argc, char* argv[])
{
	KSC_Initialize();

	FILE* f = NULL;
	fopen_s(&f, "light_shader_example.ls", "r");
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
		const char* funcNames[1];
		void* funcPtrs[1];

		funcNames[0] = "HandleArray";
		KSC_AddExternalFunction("SimpleCallee", SimpleCallee);


		if (!KSC_Compile(content, funcNames, 1, funcPtrs)) {
			printf(KSC_GetLastErrorMsg());
		}

		float ret;
		/*TestStruct1 mod = {0.0f, 1234, 0.0f};
		float (*FP)(TestStruct1* a) = (float (*)(TestStruct1* a))(intptr_t)ctx.GetJITedFuncPtr("RetSimpleValue");
		ret = FP(&mod);

		float (*FP_Dis)(float a, float b) = (float (*)(float a, float b))(intptr_t)ctx.GetJITedFuncPtr("DistanceSqr");
		ret = FP_Dis(1.5f, 6.3f);

		int (*TwoIntegerBinary)(int a, int b) = (int (*)(int a, int b))(intptr_t)ctx.GetJITedFuncPtr("TwoIntegerBinary");
		int iRet = TwoIntegerBinary(7, -3);

		float (*FloatAndIntBinary)(float a, int b) = (float (*)(float a, int b))(intptr_t)ctx.GetJITedFuncPtr("FloatAndIntBinary");
		ret = FloatAndIntBinary(7.123f, -3);

		float vData[3];
		float (*HandleVector)(float* ref) = (float (*)(float* ref))(intptr_t)ctx.GetJITedFuncPtr("HandleVector");
		ret = HandleVector(vData);*/

		struct ArrayCntr {
			float abc;
			float my_array[13];
		};
		ArrayCntr array_ref;
		void (*HandleArray)(ArrayCntr* ref) = (void (*)(ArrayCntr* ref))funcPtrs[0];
		HandleArray(&array_ref);
	}
	

	return 0;
}

