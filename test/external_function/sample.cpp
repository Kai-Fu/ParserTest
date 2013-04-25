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
	fopen_s(&f, "exntern_function.ls", "r");
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

		KSC_AddExternalFunction("SimpleCallee", SimpleCallee);

		ModuleHandle hModule = KSC_Compile(content);
		if (!hModule) {
			printf(KSC_GetLastErrorMsg());
			return -1;
		}

		struct ArrayCntr {
			float abc;
			float my_array[13];
		};
		ArrayCntr array_ref;
		FunctionHandle hFunc = KSC_GetFunctionHandleByName("HandleArray", hModule);
		void (*HandleArray)(ArrayCntr* ref) = (void (*)(ArrayCntr* ref))KSC_GetFunctionPtr(hFunc);
		HandleArray(&array_ref);
	}
	

	return 0;
}

