#include "parser_AST_Gen.h"
#include <stdio.h>

namespace SC {

Token::Token(const char* p, int num, Type tp)
{
	mpData = p;
	mNumOfChar = num;
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

Token CompilingContext::ScanForToken(const char* str, Token::Type& tp, int& charParsed, int& newLineParsed, std::string& errorMsg)
{
	newLineParsed = 0;
	tp = Token::kUnknown;
	const char* pFirstChar = str;
	const char* curIt = str;

	// Eat the white spaces and new line characters.
	//
	bool isNewLineChar = false;
	while (*curIt == ' ' || (isNewLineChar = (*curIt == '\n'))) {
		if (isNewLineChar)
			++newLineParsed;
		++curIt;
	}

	// Skip the comments, e.g. //... and /* ... */
	//
	if (*curIt == '/') {

		if (*(curIt + 1) == '*') {
			curIt += 2;
			// Seek for the end of the comments(*/)
			bool isLastAsterisk = false;
			while (*curIt != '\0') {
				if (isLastAsterisk && *curIt == '/')
					break;
				if (*curIt != '*') 
					isLastAsterisk = true;
			}
			if (*curIt == '\0') {
				errorMsg = "Comments not ended - unexpected end of file.";
				return Token(NULL, 0, Token::kUnknown);
			}
		}
		else if (*(curIt + 1) == '/') {
			curIt += 2;
			// Go to the end of the line
			while (*curIt != '\0' && *curIt != '\n') 
				++curIt;

			if (*curIt == '\n')
				newLineParsed++;
		}
	}

	Token ret(NULL, 0, Token::kUnknown);
	// Now it is expecting a token.
	//
	if (_isFirstN_Equal(curIt, "++") ||
		_isFirstN_Equal(curIt, "--") ||
		_isFirstN_Equal(curIt, "||") ||
		_isFirstN_Equal(curIt, "&&")) {

		ret = Token(curIt, 2, Token::kBinaryOp);
		curIt += 2;
	}
	else if (_isFirstN_Equal(curIt, "+") ||
			 _isFirstN_Equal(curIt, "-") ||
			 _isFirstN_Equal(curIt, "*") ||
			 _isFirstN_Equal(curIt, "/") ||
			 _isFirstN_Equal(curIt, "|") ||
			 _isFirstN_Equal(curIt, "&")) {

		ret = Token(curIt, 1, Token::kBinaryOp);
		++curIt;
	}
	else if(*curIt == '{') {
		ret = Token(curIt, 1, Token::kOpenCurly);
		++curIt;
	}
	else if(*curIt == '}') {
		ret = Token(curIt, 1, Token::kCloseCurly);
		++curIt;
	}
	else if(*curIt == '[') {
		ret = Token(curIt, 1, Token::kOpenBraket);
		++curIt;
	}
	else if(*curIt == ']') {
		ret = Token(curIt, 1, Token::kCloseBraket);
		++curIt;
	}
	else if(*curIt == '(') {
		ret = Token(curIt, 1, Token::kOpenParenthesis);
		++curIt;
	}
	else if(*curIt == ')') {
		ret = Token(curIt, 1, Token::kCloseParenthesis);
		++curIt;
	}
	else if(*curIt == ',') {
		ret = Token(curIt, 1, Token::kComma);
		++curIt;
	}
	else if(*curIt == ';') {
		ret = Token(curIt, 1, Token::kSemiColon);
		++curIt;
	}
	else if(*curIt == '.') {
		ret = Token(curIt, 1, Token::kPeriod);
		++curIt;
	}
	else if(*curIt == '!') {
		ret = Token(curIt, 1, Token::kBang);
		++curIt;
	}
	else {
		// Now handling for constants for identifiers
		bool isFirstCharNumber = _isNumber(*curIt);
		if (!isFirstCharNumber && !_isAlpha(*curIt)) {
			// Reject any unrecoginzed character
			return Token(NULL, 0, Token::kUnknown);
		}

		const char* pFirstCh = curIt;
		int idLen = 0;
		while (	*curIt != '\0' &&
				(_isAlpha(*curIt) ||
				_isNumber(*curIt) ||
				*curIt == '_')) {
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
			return Token(NULL, 0, Token::kUnknown);
		}

		if (isFirstCharNumber) {
			// check for decimal point, e.g. 123.456f
			if (*curIt == '.') {
				curIt++;
				while (_isNumber(*curIt)) 
					curIt++;
			}

			if (*curIt == 'f' || *curIt == 'F')
				curIt++;
		}
		charParsed = int(curIt - pFirstChar);

		ret = Token(pFirstCh, curIt - pFirstCh, isFirstCharNumber ? Token::kConstInt : Token::kIdentifier);
	}

	return ret;
}

CompilingContext::CompilingContext(const char* content)
{
	mCurContentPtr = content;
	mCurParsingLOC = 1;
	mIsInFuncBody = false;
}

Token CompilingContext::GetNextToken()
{

}

Token CompilingContext::PeekNextToken(int next_i)
{

}

} // namespace SC