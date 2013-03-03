#pragma once
#include <string>
#include <vector>
#include <list>
#include <hash_map>
#include "parser_defines.h"

namespace SC {


	class Token;

	enum KeyWord {
		kStructDef,
		kIf,
		kElse,
		kFor
	};

	void Initialize_AST_Gen();
	void Finish_AST_Gen();

	bool IsBuiltInType(const Token& token, VarType* out_type = NULL);
	bool IsKeyWord(const Token& token, KeyWord* out_key = NULL);



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
		int mLOC;
		Type mType;
	public:
		static Token sInvalid;
	public:
		Token(const char* p, int num, int line, Type tp);
		Token(const Token& ref);

		double GetConstValue() const;
		int GetBinaryOpLevel() const;
		Type GetType() const;

		bool IsValid() const;
		bool IsEqual(const char* dest) const;
		bool IsEqual(const Token& dest) const;
		void ToAnsiString(char* dest) const;
		std::string ToStdString() const;
	};
	
	class DataBlock
	{
	private:
		std::vector<unsigned char> mData;

	public:
		void AddFloat(Float f);
		void AddFloat2(Float f0, Float f1);
		void AddFloat3(Float f0, Float f1, Float f2);
		void AddFloat4(Float f0, Float f1, Float f2, Float f3);

		void AddInt(int i);
		void AddInt2(int i0, int i1);
		void AddInt3(int i0, int i1, int i2);
		void AddInt4(int i0, int i1, int i2, int i3);

		void* GetData();
		int GetSize();
	};

	class Operator
	{
	public:
		Operator() {}
		virtual ~Operator() {}
	};

	class Expression
	{
	public:
		Expression() {}
		virtual ~Expression() {}
	};

	class Exp_VarDef : public Expression
	{
	private:
		std::vector<std::pair<Token, DataBlock> > mVarDefs;
		VarType mVarType;
		std::string mStructName;

	public:
		Exp_VarDef(Exp_VarDef::VarType type);
		virtual ~Exp_VarDef();

		void SetStructName(const std::string& structName);
		// return variable index in this VarDef expression
		int AddVariable(const Token& var);
		int GetVariableCnt() const;
		DataBlock* GetVarDataBlock(int idx);
	};

	class Exp_StructDef : public Expression
	{
	private:
		struct Element {
			bool isStruct;
			void* type;
		};

		std::string mStructName;
		std::vector<Element> mElements;
	public:
		Exp_StructDef(std::string name);
		virtual ~Exp_StructDef();

		void AddFloat();
		void AddFloat2();
		void AddFloat3();
		void AddFloat4();
		void AddInt();
		void AddInt2();
		void AddInt3();
		void AddInt4();
		void AddStruct(const Exp_StructDef* subStruct);

		int GetStructSize() const;
		int GetElementCount() const;
		VarType GetElementType(int idx) const;
	};

	class CodeDomain : public Expression
	{
	private:
		CodeDomain* mpParentDomain;

		std::hash_map<std::string, Exp_StructDef*> mDefinedStructures;
		std::hash_map<std::string, Token> mDefinedVariables;

		std::vector<Expression*> mExpressions;

	public:
		CodeDomain();
		virtual ~CodeDomain();

		bool IsTypeDefined(const std::string& typeName);
		bool IsVariableDefined(const std::string& varName, bool includeParent);
	};

	class FunctionDomain : public CodeDomain
	{
	private:
		Token mReturnType;
		Token mFuncName;
		std::vector<Token> mArgments;
	};

	class RootDomain : public CodeDomain
	{
	private:
		std::vector<FunctionDomain*> mFunctions;
		std::hash_map<std::string, CodeDomain*> mVariableGroups;
	};



	class CompilingContext
	{
	private:
		const char* mContentPtr;
		const char* mCurParsingPtr;
		int mCurParsingLOC;

		bool mIsInFuncBody;
		std::string mCurFuncName;

		std::list<Token> mBufferedToken;
		std::list<std::pair<Token, std::string> > mErrorMessages;

		RootDomain mRootCodeDomain;
		CodeDomain* mpCurrentDomain;

	private:
		void AddErrorMessage(const Token& token, const std::string& str);
	public:
		CompilingContext(const char* content);


		Token GetNextToken();
		Token PeekNextToken(int next_i);


		bool IsVarDefinePartten(bool allowInit);
		Exp_VarDef* ParseVarDefine(bool allowInit);

		bool IsStructDefinePartten();
		Exp_StructDef* ParseStructDefine();




	private:
		Token ScanForToken(std::string& errorMsg);


	};
} // namespace SC