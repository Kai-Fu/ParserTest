// SC.cpp : Defines the entry point for the console application.
//

#include <stdio.h>
#include "parser_AST_Gen.h"

int main(int argc, char* argv[])
{
	FILE* f = fopen("light_shader_example.ls", "rb");
	if (f == NULL)
		return -1;
	fseek(f, 0, SEEK_END);
	long len = ftell(f);

	char* content = new char[len + 1];
	if (len != fread(content, 1, len, f))
		return -1;
	else {
		content[len] = '\0';
		SC::CompilingContext ctx;
		ctx.Parse(content);
	}
	return 0;
}

