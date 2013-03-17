#pragma once
#include <string>
#include <vector>
#include <list>
#include <hash_map>
#include "parser_defines.h"

#define WANT_MEM_LEAK_CHECK

namespace SC {


	class Token;
	struct TypeDesc;
	class Exp_VarDef;
	class Exp_ValueEval;
	class CodeGenValue;

	enum KeyWord {
		kStructDef,
		kIf,
		kElse,
		kFor,
		kReturn,
		kTrue,
		kFalse
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
			kUnaryOp, // !,
			kComma,
			kSemiColon,
			kPeriod,
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
		Expression();
		virtual ~Expression();
		virtual CodeGenValue* CodeGen();

#ifdef WANT_MEM_LEAK_CHECK
		static int s_expnCnt;
#endif
	};
	
	class Exp_StructDef;
	class CompilingContext;
	class Exp_FunctionDecl;

	class CodeDomain : public Expression
	{
	protected:
		CodeDomain* mpParentDomain;

		std::hash_map<std::string, Exp_StructDef*> mDefinedStructures;
		std::hash_map<std::string, Exp_VarDef*> mDefinedVariables;
		std::hash_map<std::string, Exp_FunctionDecl*> mDefinedFunctions;

		std::vector<Expression*> mExpressions;

	private:
		void AddDefinedType(Exp_StructDef* pStructDef);
		void AddDefinedVariable(const Token& t, Exp_VarDef* pDef);
		void AddDefinedFunction(Exp_FunctionDecl* pFunc);

	public:
		CodeDomain(CodeDomain* parent);
		virtual ~CodeDomain();

		CodeDomain* GetParent();

		void AddValueExpression(Exp_ValueEval* exp);
		void AddStructDefExpression(Exp_StructDef* exp);
		void AddVarDefExpression(Exp_VarDef* exp);
		void AddFunctionDefExpression(Exp_FunctionDecl* exp);
		void AddDomainExpression(CodeDomain* exp);

		bool IsTypeDefined(const std::string& typeName);
		bool IsVariableDefined(const std::string& varName, bool includeParent);
		bool IsFunctionDefined(const std::string& funcName);

		Exp_StructDef* GetStructDefineByName(const std::string& structName);
		Exp_VarDef* GetVarDefExpByName(const std::string& varName);
		Exp_FunctionDecl* GetFunctionDeclByName(const std::string& funcName);

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
		static bool Parse(CompilingContext& context, CodeDomain* curDomain, std::vector<Exp_VarDef*>& out_defs);

		void SetStructDef(Exp_StructDef* pStruct);
		Exp_ValueEval* GetVarInitExp();
		Token GetVarName() const;
		VarType GetVarType() const;
		Exp_StructDef* GetStructDef();
	};

	class Exp_StructDef : public CodeDomain
	{
	private:
		std::string mStructName;
		std::hash_map<int, Exp_VarDef*> mIdx2ValueDefs;
	public:
		Exp_StructDef(std::string name, CodeDomain* parentDomain);
		virtual ~Exp_StructDef();
		static Exp_StructDef* Parse(CompilingContext& context, CodeDomain* curDomain);

		int GetStructSize() const;
		int GetElementCount() const;
		const std::string& GetStructureName() const;
		VarType GetElementType(int idx, const Exp_StructDef* &outStructDef) const;
	};

	class Exp_ValueEval : public Expression
	{
	public:
		struct TypeInfo {
			VarType type;
			Exp_StructDef* pStructDef;
			bool IsAssignable(const TypeInfo& from, bool& FtoI);
		};

		virtual bool CheckSemantic(TypeInfo& outType, std::string& errMsg, std::vector<std::string>& warnMsg) = 0;
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
		virtual bool CheckSemantic(TypeInfo& outType, std::string& errMsg, std::vector<std::string>& warnMsg);
	};

	class Exp_TrueOrFalse : public Exp_ValueEval
	{
	private:
		bool mValue;
	public:
		Exp_TrueOrFalse(bool value);
		virtual ~Exp_TrueOrFalse();

		bool GetValue() const;
		virtual bool CheckSemantic(TypeInfo& outType, std::string& errMsg, std::vector<std::string>& warnMsg);
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

		virtual bool CheckSemantic(TypeInfo& outType, std::string& errMsg, std::vector<std::string>& warnMsg);
	};

	class Exp_BuiltInInitializer : public Exp_ValueEval
	{
	private:
		Exp_ValueEval* mpSubExprs[4];
		VarType mType;

	public:
		Exp_BuiltInInitializer(Exp_ValueEval** pExp, int cnt, VarType tp);
		virtual ~Exp_BuiltInInitializer();

		virtual bool CheckSemantic(TypeInfo& outType, std::string& errMsg, std::vector<std::string>& warnMsg);
	};

	class Exp_UnaryOp : public Exp_ValueEval
	{
	private:
		std::string mOpType;
		Exp_ValueEval* mpExpr;

	public:
		Exp_UnaryOp(const std::string& op, Exp_ValueEval* pExp);
		virtual ~Exp_UnaryOp();

		virtual bool CheckSemantic(TypeInfo& outType, std::string& errMsg, std::vector<std::string>& warnMsg);
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

		virtual bool CheckSemantic(TypeInfo& outType, std::string& errMsg, std::vector<std::string>& warnMsg);
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

		virtual bool CheckSemantic(TypeInfo& outType, std::string& errMsg, std::vector<std::string>& warnMsg);
	};

	class Exp_FunctionDecl : public CodeDomain
	{
	public:
		struct ArgDesc {
			Exp_ValueEval::TypeInfo typeInfo;
			bool isByRef;
			Token token;
		};

	private:
		VarType mReturnType;
		Exp_StructDef* mpRetStruct;
		std::string mFuncName;
		std::vector<ArgDesc> mArgments;
	public:
		Exp_FunctionDecl(CodeDomain* parent);
		virtual ~Exp_FunctionDecl();
		const std::string GetFunctionName() const;
		VarType GetReturnType(Exp_StructDef* &retStruct);
		int GetArgumentCnt() const;
		ArgDesc* GetArgumentDesc(int idx);
		bool HasSamePrototype(const Exp_FunctionDecl& ref) const;

		static bool Parse(CompilingContext& context, CodeDomain* curDomain);
	};

	class Exp_FuncRet : public Exp_ValueEval
	{
	private:
		Exp_ValueEval* mpRetValue;
		Exp_FunctionDecl* mpFuncDecl;
	public:
		Exp_FuncRet(Exp_FunctionDecl* pFuncDecl, Exp_ValueEval* pRet);
		virtual ~Exp_FuncRet();

		virtual bool CheckSemantic(TypeInfo& outType, std::string& errMsg, std::vector<std::string>& warnMsg);
	};

	class Exp_FunctionCall : public Exp_ValueEval
	{
	private:
		std::vector<Exp_ValueEval*> mInputArgs;
		Exp_FunctionDecl* mpFuncDef;
	public:
		Exp_FunctionCall(Exp_FunctionDecl* pFuncDef, Exp_ValueEval** ppArgs, int cnt);
		virtual ~Exp_FunctionCall();

		virtual bool CheckSemantic(TypeInfo& outType, std::string& errMsg, std::vector<std::string>& warnMsg);
	};

	class Exp_If : public Expression
	{
	private:
		CodeDomain mIfDomain;
		CodeDomain mElseDomain;
	public:
		Exp_If();
		virtual ~Exp_If();

	};

	class RootDomain : public CodeDomain
	{
	private:
		
	
	public:
		RootDomain();
		virtual ~RootDomain();
	};



	class CompilingContext
	{
	public:
		enum ParsingStatus {
			kAllowValueExp		= 0x00000001,
			kAllowVarInit		= 0x00000002,
			kAllowVarDef		= 0x00000004,
			kAllowReturnExp		= 0x00000008,
			kAlllowStructDef	= 0x00000010,
			kAlllowFuncDef		= 0x00000020
		};
	private:
		const char* mContentPtr;
		const char* mCurParsingPtr;
		int mCurParsingLOC;


		std::list<Token> mBufferedToken;
		std::list<std::pair<Token, std::string> > mErrorMessages;
		std::list<std::pair<Token, std::string> > mWarningMessages;

		RootDomain* mRootCodeDomain;
		std::list<int> mStatusCode;

	private:
		bool IsVarDefinePartten(bool allowInit);
		bool IsStructDefinePartten();
		bool IsFunctionDefinePartten();

		bool ParseSingleExpression(CodeDomain* curDomain, const char* endT);
		
		Token ScanForToken(std::string& errorMsg);

	public:
		CompilingContext(const char* content);
		~CompilingContext();

		void AddErrorMessage(const Token& token, const std::string& str);
		void AddWarningMessage(const Token& token, const std::string& str);
		bool IsEOF() const;

		Token GetNextToken();
		Token PeekNextToken(int next_i);
		bool ExpectAndEat(const char* str);
		bool ExpectTypeAndEat(CodeDomain* curDomain, VarType& outType, Exp_StructDef*& outStructDef);

		bool Parse(const char* content);

		void PushStatusCode(int code);
		int GetStatusCode();
		void PopStatusCode();

		bool ParseCodeDomain(CodeDomain* curDomain, const char* endT);
		// Simple expression means the expression is in the bracket pair or it is a expression other than a binary operation.
		//
		Exp_ValueEval* ParseSimpleExpression(CodeDomain* curDomain);
		// Complex expression means it contains any simple expression and binary operation.
		//
		Exp_ValueEval* ParseComplexExpression(CodeDomain* curDomain, const char* pEndToken);

	};
} // namespace SC