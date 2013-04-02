// SC.cpp : Defines the entry point for the console application.
//

#include <stdio.h>
#include "parser_AST_Gen.h"
#include "IR_Gen_Context.h"

struct TestStruct1
{
	float a;
	int cond;
	float b;
};


int main(int argc, char* argv[])
{
	SC::Initialize_AST_Gen();
	SC::InitializeCodeGen();

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
		SC::CompilingContext ctx(content);
		ctx.Parse(content);
		/*while (1) {
			SC::Token curT = ctx.GetNextToken();
			if (curT.IsValid())
				printf("%s\n", curT.ToStdString().c_str());
			else
				break;

		}*/
		ctx.JIT_Compile();
		TestStruct1 mod = {0.0f, 1234, 0.0f};
		float (*FP)(TestStruct1* a) = (float (*)(TestStruct1* a))(intptr_t)ctx.GetJITedFuncPtr("RetSimpleValue");
		float ret = FP(&mod);

		float (*FP_Dis)(float a, float b) = (float (*)(float a, float b))(intptr_t)ctx.GetJITedFuncPtr("DistanceSqr");
		ret = FP_Dis(1.5f, 6.3f);

		int (*TwoIntegerBinary)(int a, int b) = (int (*)(int a, int b))(intptr_t)ctx.GetJITedFuncPtr("TwoIntegerBinary");
		int iRet = TwoIntegerBinary(7, -3);

		float (*FloatAndIntBinary)(float a, int b) = (float (*)(float a, int b))(intptr_t)ctx.GetJITedFuncPtr("FloatAndIntBinary");
		ret = FloatAndIntBinary(7.123f, -3);

		float vData[3];
		float (*HandleVector)(float* ref) = (float (*)(float* ref))(intptr_t)ctx.GetJITedFuncPtr("HandleVector");
		ret = HandleVector(vData);
	}
	

	SC::Finish_AST_Gen();
	return 0;
}

