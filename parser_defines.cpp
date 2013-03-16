#include "parser_defines.h"

namespace SC {

bool IsBuiltInType(VarType type)
{
	switch (type) {
	case kFloat:
	case kFloat2:
	case kFloat3:
	case kFloat4:
	case kInt:
	case kInt2:
	case kInt3:
	case kInt4:
		return true;
	}
	return false;
}

bool IsIntegerType(VarType type)
{
	switch (type) {
	case kFloat:
	case kFloat2:
	case kFloat3:
	case kFloat4:
		return false;
	case kInt:
	case kInt2:
	case kInt3:
	case kInt4:
		return true;
	}
	return false;
}

int TypeElementCnt(VarType type)
{
	switch (type) {
	case kFloat:
		return 1;
	case kFloat2:
		return 2;
	case kFloat3:
		return 3;
	case kFloat4:
		return 4;
	case kInt:
		return 1;
	case kInt2:
		return 2;
	case kInt3:
		return 3;
	case kInt4:
		return 4;
	}
	return 0x0ffffff;
}

int TypeSize(VarType type)
{
	switch (type) {
	case kFloat:
		return 1*sizeof(Float);
	case kFloat2:
		return 2*sizeof(Float);
	case kFloat3:
		return 3*sizeof(Float);
	case kFloat4:
		return 4*sizeof(Float);
	case kInt:
		return 1*sizeof(Int);
	case kInt2:
		return 2*sizeof(Int);
	case kInt3:
		return 3*sizeof(Int);
	case kInt4:
		return 4*sizeof(Int);
	}
	return 0;
}

VarType MakeType(bool I_or_F, int elemCnt)
{
	if (I_or_F) {
		return VarType(VarType::kInt + elemCnt - 1);
	}
	else {
		return VarType(VarType::kFloat + elemCnt - 1);
	}
}

int ConvertSwizzle(const char* swizzleStr, int swizzleIdx[4])
{
	int cnt = 0;
	const char* pCur = swizzleStr;
	while (*pCur != '\0') {
		switch (*pCur) {
		case 'x':
		case 'r':
			swizzleIdx[cnt] = 0;
			break;
		case 'y':
		case 'g':
			swizzleIdx[cnt] = 1;
			break;
		case 'z':
		case 'b':
			swizzleIdx[cnt] = 2;
			break;
		case 'w':
		case 'a':
			swizzleIdx[cnt] = 3;
			break;
		default:
			return -1;
		}
		++pCur;
		++cnt;
		if (cnt > 4)
			return -1;
	}
	return cnt;
}

} // namespace SC