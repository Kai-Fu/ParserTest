#pragma once
#define MAX_TOKEN_LENGTH 100
#include "SC_API.h"

namespace SC {

	bool IsBuiltInType(VarType type);
	bool IsFloatType(VarType type);
	bool IsIntegerType(VarType type);
	bool IsValueType(VarType type);
	int TypeElementCnt(VarType type);
	int TypeSize(VarType type);
	VarType MakeType(bool I_or_F, int elemCnt);
	int ConvertSwizzle(const char* swizzleStr, int swizzleIdx[4]);
	bool IsTypeCompatible(VarType dest, VarType from, bool& FtoIwarning);
} // namespace SC