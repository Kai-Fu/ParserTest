#pragma once
#define MAX_TOKEN_LENGTH 100

namespace SC {

	typedef float Float;
	
	enum VarType {
		kFloat,
		kFloat2,
		kFloat3,
		kFloat4,
		kInt,
		kInt2,
		kInt3,
		kInt4,
		kStructure,
		kInvalid
		};
} // namespace SC