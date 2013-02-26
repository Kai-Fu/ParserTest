#pragma once
#include <string>
#include <vector>
#include <queue>

#define MAX_TOKEN_LENGTH 100

namespace SC {

	class Token
	{
	public:
		enum Type {
			kIdentifier,
			kConstInt,
			kConstFloat,
			kOpenCurly, // {
			kCloseCurly, // }
			kOpenParenthesis, // (
			kCloseParenthesis, // )
			kOpenBraket, // [
			kCloseBraket, // ]
			kBinaryOp, // +,=,*,/, etc
			kUnaryOp, // !, ++, --, etc
			kComma,
			kSemiColon,
			kPeriod,
			kBang,
			kUnknown
		};
	private:
		const char* mpData;
		int mNumOfChar;
		Type mType;

	public:
		Token(const char* p, int num, Type tp);
		Token(const Token& ref);

		double GetConstValue() const;
		int GetBinaryOpLevel() const;

		bool IsEqual(const char* dest) const;
		bool IsEqual(const Token& dest) const;
	};


	class CompilingContext
	{
	private:
		const char* mCurContentPtr;
		int mCurParsingLOC;

		bool mIsInFuncBody;
		std::string mCurFuncName;

		std::queue<Token> mBufferedToken;
	public:
		CompilingContext(const char* content);

		Token GetNextToken();
		Token PeekNextToken(int next_i);

		static Token ScanForToken(const char* str, Token::Type& tp, int& charParsed, int& newLineParsed, std::string& errorMsg);


	};
} // namespace SC