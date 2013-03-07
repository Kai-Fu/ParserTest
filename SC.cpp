// SC.cpp : Defines the entry point for the console application.
//

#include <stdio.h>
#include "parser_AST_Gen.h"

int main(int argc, char* argv[])
{
	FILE* f = NULL;
	fopen_s(&f, "light_shader_example.ls", "r");
	if (f == NULL)
		return -1;
	fseek(f, 0, SEEK_END);
	long len = ftell(f);
	fseek(f, 0, SEEK_SET);

	char* content = new char[len + 1];
	size_t readSize = fread(content, 1, len, f);
	if (readSize == 0)
		return -1;
	else {
		content[readSize] = '\0';
		SC::CompilingContext ctx(content);
		//ctx.Parse(content);
		while (1) {
			SC::Token curT = ctx.GetNextToken();
			if (curT.IsValid())
				printf("%s\n", curT.ToStdString().c_str());
			else
				break;

		}
	}
	return 0;
}

