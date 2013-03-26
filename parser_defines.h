#pragma once
#define MAX_TOKEN_LENGTH 100

namespace SC {

#ifdef WANT_DOUBLE_FLOAT
	typedef double Float;
#else
	typedef float Float;
#endif
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
	bool IsFloatType(VarType type);
	bool IsIntegerType(VarType type);
	int TypeElementCnt(VarType type);
	int TypeSize(VarType type);
	VarType MakeType(bool I_or_F, int elemCnt);
	int ConvertSwizzle(const char* swizzleStr, int swizzleIdx[4]);
	bool IsTypeCompatible(VarType dest, VarType from, bool& FtoIwarning);
} // namespace SC