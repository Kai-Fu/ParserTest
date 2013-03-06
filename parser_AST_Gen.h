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
		Token();
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
	
	class Exp_StructDef;
	class CompilingContext;

	class CodeDomain : public Expression
	{
	private:
		CodeDomain* mpParentDomain;

		std::hash_map<std::string, Exp_StructDef*> mDefinedStructures;
		std::hash_map<std::string, Token> mDefinedVariables;

		std::vector<Expression*> mExpressions;

	public:
		CodeDomain(CodeDomain* parent);
		virtual ~CodeDomain();

		void AddExpression(Expression* exp);
		void AddDefinedType(Exp_StructDef* pStructDef);
		bool IsTypeDefined(const std::string& typeName);

		void AddDefinedVariable(const Token& t);
		bool IsVariableDefined(const std::string& varName, bool includeParent);

		Exp_StructDef* GetStructDefineByName(const std::string& name);
	};

	class Exp_VarDef : public Expression
	{
	private:
		std::vector<std::pair<Token, DataBlock*> > mVarDefs;
		VarType mVarType;
		Exp_StructDef* mpStructDef;

	public:
		Exp_VarDef(VarType type);
		virtual ~Exp_VarDef();
		static Exp_VarDef* Parse(CompilingContext& context, CodeDomain* curDomain, bool allowInit);

		void SetStructDef(Exp_StructDef* pStruct);
		// return variable index in this VarDef expression
		int AddVariable(const Token& var, DataBlock* pData = NULL);
		int GetVariableCnt() const;
		DataBlock* GetVarDataBlock(int idx);
		Token GetVarName(int idx) const;
		VarType GetVarType() const;
		Exp_StructDef* GetStructDef();
	};

	class Exp_StructDef : public Expression
	{
	private:
		struct Element {
			bool isStruct;
			void* type;
			std::string name;
		};
		std::string mStructName;
		std::vector<Element> mElements;
		CodeDomain mStructDomain;
	public:
		Exp_StructDef(std::string name, CodeDomain* parentDomain);
		virtual ~Exp_StructDef();
		static Exp_StructDef* Parse(CompilingContext& context, CodeDomain* curDomain);

		void AddElement(const std::string& varName, VarType type, Exp_StructDef* pStructDef);
		int GetStructSize() const;
		int GetElementCount() const;
		const std::string& GetStructureName() const;
		VarType GetElementType(int idx) const;
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
	
	public:
		RootDomain();
		virtual ~RootDomain();
	};



	class CompilingContext
	{
	private:
		const char* mContentPtr;
		const char* mCurParsingPtr;
		int mCurParsingLOC;


		std::list<Token> mBufferedToken;
		std::list<std::pair<Token, std::string> > mErrorMessages;

		RootDomain* mRootCodeDomain;

	private:
		bool ParseSingleExpression(CodeDomain* curDomain);
		bool IsVarDefinePartten(bool allowInit, CodeDomain* curDomain);
		bool IsStructDefinePartten();
		bool ParseCodeDomain(CodeDomain* curDomain);
		Token ScanForToken(std::string& errorMsg);

	public:
		CompilingContext();
		~CompilingContext();

		void AddErrorMessage(const Token& token, const std::string& str);
		bool IsEOF() const;

		Token GetNextToken();
		Token PeekNextToken(int next_i);

		bool Parse(const char* content);


	};
} // namespace SC