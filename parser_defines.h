#pragma once
#define MAX_TOKEN_LENGTH 100

namespace SC {

	typedef float Float;
	typedef int Int;
	
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
		kInvalid
		};

	bool IsBuiltInType(VarType type);
	bool IsIntegerType(VarType type);
	int TypeElementCnt(VarType type);
	int TypeSize(VarType type);
	VarType MakeType(bool I_or_F, int elemCnt);
	int ConvertSwizzle(const char* swizzleStr, int swizzleIdx[4]);
} // namespace SC