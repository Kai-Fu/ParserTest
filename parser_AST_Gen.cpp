#include "parser_AST_Gen.h"
#include <stdio.h>

namespace SC {

Token Token::sInvalid = Token(NULL, 0, -1, Token::kUnknown);

Token::Token(const char* p, int num, int line, Type tp)
{
	mpData = p;
	mNumOfChar = num;
	mLOC = line;
	mType = tp;
}

Token::Token(const Token& ref)
{
	mpData = ref.mpData;
	mNumOfChar = ref.mNumOfChar;
	mType = ref.mType;
}

double Token::GetConstValue() const
{
	if (mType == kConstInt || mType == kConstFloat) {
		char tempString[MAX_TOKEN_LENGTH+1];
		memcpy(tempString, mpData, mNumOfChar*sizeof(char));
		tempString[mNumOfChar] = '\0';
		return atof(tempString);
	}
	return 0.0;
}

int Token::GetBinaryOpLevel() const
{
	if (mType != kBinaryOp)
		return 0;

	if (mNumOfChar == 1) {
		switch (*mpData) {
		case '|':
		case '&':
		case '+':
		case '-':
			return 100;
		case '*':
		case '/':
			return 200;
		case '=':
			return 50;
		}
	}
	else {
		if (IsEqual("||") || IsEqual("&&"))
			return 70;
		else if (IsEqual("=="))
			return 80;
	}

	return 0;
}

bool Token::IsValid() const
{
	return (mNumOfChar > 0 && mpData != NULL);
}

bool Token::IsEqual(const char* dest) const
{
	size_t len = strlen(dest);
	if (len != mNumOfChar)
		return false;
	for (int i = 0; i < mNumOfChar; ++i) {
		if (dest[i] != mpData[i])
			return false;
	}
	return true;
}

bool Token::IsEqual(const Token& dest) const
{
	size_t len = dest.mNumOfChar;
	if (len != mNumOfChar)
		return false;
	for (int i = 0; i < mNumOfChar; ++i) {
		if (dest.mpData[i] != mpData[i])
			return false;
	}
	return true;
}

void Token::ToAnsiString(char* dest) const
{
	int i = 0;
	for (; i < mNumOfChar; ++i)
		dest[i] = mpData[i];
	dest[i] = '\0';
}

static bool _isAlpha(char ch)
{
	return ((ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z'));
}

static bool _isNumber(char ch)
{
	return (ch >= '0' && ch <= '9');
}

static bool _isFirstN_Equal(const char* test_str, const char* dest)
{
	int i = 0;
	while (test_str[i] != '\0' && dest[i] != '\0') {
		if (test_str[i] != dest[i])
			return false;
		++i;
	}

	if (test_str[i] == '\0' && dest[i] != '\0')
		return false;

	return true;
}

Token CompilingContext::ScanForToken(std::string& errorMsg)
{
	// Eat the white spaces and new line characters.
	//
	bool isNewLineChar = false;
	while (*mCurParsingPtr == ' ' || (isNewLineChar = (*mCurParsingPtr == '\n'))) {
		if (isNewLineChar)
			++mCurParsingLOC;
		++mCurParsingPtr;
	}

	// Skip the comments, e.g. //... and /* ... */
	//
	if (*mCurParsingPtr == '/') {

		if (*(mCurParsingPtr + 1) == '*') {
			mCurParsingPtr += 2;
			// Seek for the end of the comments(*/)
			bool isLastAsterisk = false;
			while (*mCurParsingPtr != '\0') {
				if (isLastAsterisk && *mCurParsingPtr == '/')
					break;
				if (*mCurParsingPtr != '*') 
					isLastAsterisk = true;
			}
			if (*mCurParsingPtr == '\0') {
				errorMsg = "Comments not ended - unexpected end of file.";
				return Token::sInvalid;
			}
		}
		else if (*(mCurParsingPtr + 1) == '/') {
			mCurParsingPtr += 2;
			// Go to the end of the line
			while (*mCurParsingPtr != '\0' && *mCurParsingPtr != '\n') 
				++mCurParsingPtr;

			if (*mCurParsingPtr == '\n')
				mCurParsingLOC++;
		}
	}

	Token ret = Token::sInvalid;
	// Now it is expecting a token.
	//
	if (_isFirstN_Equal(mCurParsingPtr, "++") ||
		_isFirstN_Equal(mCurParsingPtr, "--") ||
		_isFirstN_Equal(mCurParsingPtr, "||") ||
		_isFirstN_Equal(mCurParsingPtr, "&&")) {

		ret = Token(mCurParsingPtr, 2, mCurParsingLOC, Token::kBinaryOp);
		mCurParsingPtr += 2;
	}
	else if (_isFirstN_Equal(mCurParsingPtr, "+") ||
			 _isFirstN_Equal(mCurParsingPtr, "-") ||
			 _isFirstN_Equal(mCurParsingPtr, "*") ||
			 _isFirstN_Equal(mCurParsingPtr, "/") ||
			 _isFirstN_Equal(mCurParsingPtr, "|") ||
			 _isFirstN_Equal(mCurParsingPtr, "&")) {

		ret = Token(mCurParsingPtr, 1, mCurParsingLOC, Token::kBinaryOp);
		++mCurParsingPtr;
	}
	else if(*mCurParsingPtr == '{') {
		ret = Token(mCurParsingPtr, 1, mCurParsingLOC, Token::kOpenCurly);
		++mCurParsingPtr;
	}
	else if(*mCurParsingPtr == '}') {
		ret = Token(mCurParsingPtr, 1, mCurParsingLOC, Token::kCloseCurly);
		++mCurParsingPtr;
	}
	else if(*mCurParsingPtr == '[') {
		ret = Token(mCurParsingPtr, 1, mCurParsingLOC, Token::kOpenBraket);
		++mCurParsingPtr;
	}
	else if(*mCurParsingPtr == ']') {
		ret = Token(mCurParsingPtr, 1, mCurParsingLOC, Token::kCloseBraket);
		++mCurParsingPtr;
	}
	else if(*mCurParsingPtr == '(') {
		ret = Token(mCurParsingPtr, 1, mCurParsingLOC, Token::kOpenParenthesis);
		++mCurParsingPtr;
	}
	else if(*mCurParsingPtr == ')') {
		ret = Token(mCurParsingPtr, 1, mCurParsingLOC, Token::kCloseParenthesis);
		++mCurParsingPtr;
	}
	else if(*mCurParsingPtr == ',') {
		ret = Token(mCurParsingPtr, 1, mCurParsingLOC, Token::kComma);
		++mCurParsingPtr;
	}
	else if(*mCurParsingPtr == ';') {
		ret = Token(mCurParsingPtr, 1, mCurParsingLOC, Token::kSemiColon);
		++mCurParsingPtr;
	}
	else if(*mCurParsingPtr == '.') {
		ret = Token(mCurParsingPtr, 1, mCurParsingLOC, Token::kPeriod);
		++mCurParsingPtr;
	}
	else if(*mCurParsingPtr == '!') {
		ret = Token(mCurParsingPtr, 1, mCurParsingLOC, Token::kBang);
		++mCurParsingPtr;
	}
	else {
		// Now handling for constants for identifiers
		bool isFirstCharNumber = _isNumber(*mCurParsingPtr);
		if (!isFirstCharNumber && (!_isAlpha(*mCurParsingPtr) || *mCurParsingPtr != '_')) {
			// Unrecoginzed character
			return Token(mCurParsingPtr++, 1, mCurParsingLOC, Token::kUnknown);
		}

		const char* pFirstCh = mCurParsingPtr;
		int idLen = 0;
		while (	*mCurParsingPtr != '\0' &&
				(_isAlpha(*mCurParsingPtr) ||
				_isNumber(*mCurParsingPtr) ||
				*mCurParsingPtr == '_')) {
			idLen++;
		}
		bool hasNonNumber = false;
		for (int i = 0; i < idLen; ++i) {
			if (!_isNumber(pFirstCh[i]))
				hasNonNumber = true;
		}

		if (isFirstCharNumber && hasNonNumber) {
			errorMsg = "Invalid identifier - ";
			errorMsg.append(pFirstCh, idLen);
			return Token(NULL, 0, mCurParsingLOC, Token::kUnknown);
		}

		if (isFirstCharNumber) {
			// check for decimal point, e.g. 123.456f
			if (*mCurParsingPtr == '.') {
				mCurParsingPtr++;
				while (_isNumber(*mCurParsingPtr)) 
					mCurParsingPtr++;
			}

			if (*mCurParsingPtr == 'f' || *mCurParsingPtr == 'F')
				mCurParsingPtr++;
		}
		
		ret = Token(pFirstCh, mCurParsingPtr - pFirstCh, mCurParsingLOC, isFirstCharNumber ? Token::kConstInt : Token::kIdentifier);
	}

	return ret;
}

CompilingContext::CompilingContext(const char* content)
{
	mContentPtr = content;
	mCurParsingPtr = mContentPtr;
	mCurParsingLOC = 1;
	mIsInFuncBody = false;
}

Token CompilingContext::GetNextToken()
{
	Token ret = PeekNextToken(0);
	if (ret.IsValid()) {
		mBufferedToken.erase(mBufferedToken.begin());
	}
	return ret;
}

Token CompilingContext::PeekNextToken(int next_i)
{
	int charParsed = 0;
	int lineParsed = 0;

	int tokenNeeded = 0;
	if ((int)mBufferedToken.size() <= next_i) 
		tokenNeeded = next_i -  mBufferedToken.size() + 1;
	
	for (int i = 0; i < tokenNeeded; ++i) {
		std::string errMsg;
		Token ret = ScanForToken(errMsg);

		if (ret.IsValid()) {
			mBufferedToken.push_back(ret);
		}
		else {
			mErrorMessages.push_back(errMsg);
			break;
		}
	}

	if ((int)mBufferedToken.size() > next_i) {
		std::list<Token>::iterator it = mBufferedToken.begin();
		int itCnt = next_i;
		while (itCnt-- > 0) ++it;
		return *it;
	}
	else 
		return Token::sInvalid;
}

static std::hash_map<std::string, VarType> s_BuiltInTypes;
static std::hash_map<std::string, KeyWord> s_KeyWords;

void Initialize_AST_Gen()
{
	s_BuiltInTypes["float"] = kFloat;
	s_BuiltInTypes["float2"] = kFloat2;
	s_BuiltInTypes["float3"] = kFloat3;
	s_BuiltInTypes["float4"] = kFloat4;

	s_BuiltInTypes["int"] = kInt;
	s_BuiltInTypes["int2"] = kInt2;
	s_BuiltInTypes["int3"] = kInt3;
	s_BuiltInTypes["int4"] = kInt4;

	s_KeyWords["struct"] = kStructDef;
	s_KeyWords["if"] = kIf;
	s_KeyWords["else"] = kElse;
	s_KeyWords["for"] = kFor;
}

void Finish_AST_Gen()
{
	s_BuiltInTypes.clear();
	s_KeyWords.clear();
}

bool IsBuiltInType(const Token& token, VarType& out_type)
{
	char tempString[MAX_TOKEN_LENGTH];
	token.ToAnsiString(tempString);
	std::hash_map<std::string, VarType>::iterator it = s_BuiltInTypes.find(tempString);
	if (it != s_BuiltInTypes.end()) {
		out_type = it->second;
		return true;
	}
	else
		return false;
}

bool IsKeyWord(const Token& token, KeyWord& out_key)
{
	char tempString[MAX_TOKEN_LENGTH];
	token.ToAnsiString(tempString);
	std::hash_map<std::string, KeyWord>::iterator it = s_KeyWords.find(tempString);
	if (it != s_KeyWords.end()) {
		out_key = it->second;
		return true;
	}
	else
		return false;
}

} // namespace SC