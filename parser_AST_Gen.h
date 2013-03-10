#pragma once
#include <string>
#include <vector>
#include <list>
#include <hash_map>
#include "parser_defines.h"

namespace SC {


	class Token;
	struct TypeDesc;
	class Exp_VarDef;
	class Exp_ValueEval;

	enum KeyWord {
		kStructDef,
		kIf,
		kElse,
		kFor
	};

	void Initialize_AST_Gen();
	void Finish_AST_Gen();

	bool IsBuiltInType(const Token& token, TypeDesc* out_type = NULL);
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
		static Token sEOF;
	public:
		Token();
		Token(const char* p, int num, int line, Type tp);
		Token(const Token& ref);

		double GetConstValue() const;
		int GetBinaryOpLevel() const;
		Type GetType() const;

		bool IsValid() const;
		bool IsEOF() const;
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
		std::hash_map<std::string, Exp_VarDef*> mDefinedVariables;

		std::vector<Expression*> mExpressions;

	public:
		CodeDomain(CodeDomain* parent);
		virtual ~CodeDomain();

		void AddExpression(Expression* exp);
		void AddDefinedType(Exp_StructDef* pStructDef);
		bool IsTypeDefined(const std::string& typeName);

		void AddDefinedVariable(const Token& t, Exp_VarDef* pDef);
		bool IsVariableDefined(const std::string& varName, bool includeParent);

		Exp_StructDef* GetStructDefineByName(const std::string& structName);
		Exp_VarDef* GetVarDefExpByName(const std::string& varName);
	};

	class Exp_VarDef : public Expression
	{
	private:
		Token mVarName;
		Exp_ValueEval* mpInitValue;
		VarType mVarType;
		Exp_StructDef* mpStructDef;

	public:
		Exp_VarDef(VarType type, const Token& var, Exp_ValueEval* pInitValue);
		virtual ~Exp_VarDef();
		static bool Parse(CompilingContext& context, CodeDomain* curDomain, bool allowInit, std::vector<Exp_VarDef*>& out_defs);

		void SetStructDef(Exp_StructDef* pStruct);
		Exp_ValueEval* GetVarInitExp();
		Token GetVarName() const;
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

	class Exp_ValueEval : public Expression
	{
	public:
		virtual VarType GetValueType() = 0;
	};

	class Exp_Constant : public Exp_ValueEval
	{
	private:
		double mValue;
		bool mIsFromFloat;

	public:
		Exp_Constant(double v, bool f);
		virtual ~Exp_Constant();

		double GetValue() const;
		virtual VarType GetValueType();
	};

	class Exp_VariableRef : public Exp_ValueEval
	{
	private:
		Token mVariable;
		Exp_VarDef* mpDef;

	public:
		Exp_VariableRef(Token t, Exp_VarDef* pDef);
		virtual ~Exp_VariableRef();
		Exp_StructDef* GetStructDef();

		virtual VarType GetValueType();
	};

	class Exp_BuiltInInitializer : public Exp_ValueEval
	{
	private:
		Exp_ValueEval* mpSubExprs[4];
		VarType mType;

	public:
		Exp_BuiltInInitializer(Exp_ValueEval** pExp, int cnt, VarType tp);
		virtual ~Exp_BuiltInInitializer();

		virtual VarType GetValueType();
	};

	class Exp_UnaryOp : public Exp_ValueEval
	{
	private:
		std::string mOpType;
		Exp_ValueEval* mpExpr;

	public:
		Exp_UnaryOp(const std::string& op, Exp_ValueEval* pExp);
		virtual ~Exp_UnaryOp();

		virtual VarType GetValueType();
	};

	class Exp_BinaryOp : public Exp_ValueEval
	{
	private:
		std::string mOperator;
		Exp_ValueEval* mpLeftExp;
		Exp_ValueEval* mpRightExp;

	public:
		Exp_BinaryOp(const std::string& op, Exp_ValueEval* pLeft, Exp_ValueEval* pRight);
		virtual ~Exp_BinaryOp();

		virtual VarType GetValueType();
	};

	// A DotOp is either to access the structure member or to perform swizzle for a built-in type
	//
	class Exp_DotOp : public Exp_ValueEval
	{
	private:
		std::string mOpStr;
		Exp_ValueEval* mpExp;
	public:
		Exp_DotOp(const std::string& opStr, Exp_ValueEval* pExp);
		virtual ~Exp_DotOp();

		virtual VarType GetValueType();
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
		bool IsVarDefinePartten(bool allowInit);
		bool IsStructDefinePartten();

		bool ParseSingleExpression(CodeDomain* curDomain);
		bool ParseCodeDomain(CodeDomain* curDomain);
		
		Token ScanForToken(std::string& errorMsg);
		bool ExpectAndEat(const char* str);

	public:
		CompilingContext(const char* content);
		~CompilingContext();

		void AddErrorMessage(const Token& token, const std::string& str);
		bool IsEOF() const;

		Token GetNextToken();
		Token PeekNextToken(int next_i);

		bool Parse(const char* content);

		// Simple expression means the expression is in the bracket pair or it is a expression other than a binary operation.
		//
		Exp_ValueEval* ParseSimpleExpression(CodeDomain* curDomain);
		// Complex expression means it contains any simple expression and binary operation.
		//
		Exp_ValueEval* ParseComplexExpression(CodeDomain* curDomain, const char* pEndToken);

	};
} // namespace SC