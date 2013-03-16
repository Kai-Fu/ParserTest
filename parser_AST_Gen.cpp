#include "parser_AST_Gen.h"
#include <stdio.h>
#include <assert.h>

namespace SC {

Token Token::sInvalid = Token(NULL, 0, 0, Token::kUnknown);
Token Token::sEOF = Token(NULL, -1, -1, Token::kUnknown);

static std::hash_map<std::string, TypeDesc> s_BuiltInTypes;
static std::hash_map<std::string, KeyWord> s_KeyWords;

struct TypeDesc {
	VarType type;
	int elemCnt;
	bool isInt;

	TypeDesc() {type = VarType::kInvalid; elemCnt = 0; isInt = false;}
	TypeDesc(VarType tp, int cnt, bool i) {type = tp; elemCnt = cnt; isInt = i;}
};

void Initialize_AST_Gen()
{
	s_BuiltInTypes["float"] = TypeDesc(kFloat, 1, false);
	s_BuiltInTypes["float2"] = TypeDesc(kFloat2, 2, false);
	s_BuiltInTypes["float3"] = TypeDesc(kFloat3, 3, false);
	s_BuiltInTypes["float4"] = TypeDesc(kFloat4, 4, false);

	s_BuiltInTypes["int"] = TypeDesc(kInt, 1, true);
	s_BuiltInTypes["int2"] = TypeDesc(kInt2, 2, true);
	s_BuiltInTypes["int3"] = TypeDesc(kInt3, 3, true);
	s_BuiltInTypes["int4"] = TypeDesc(kInt4, 4, true);

	s_BuiltInTypes["bool"] = TypeDesc(kBoolean, 4, true);

	s_KeyWords["struct"] = kStructDef;
	s_KeyWords["if"] = kIf;
	s_KeyWords["else"] = kElse;
	s_KeyWords["for"] = kFor;
	s_KeyWords["return"] = kFor;
	s_KeyWords["true"] = kTrue;
	s_KeyWords["false"] = kFalse;
}

void Finish_AST_Gen()
{
	s_BuiltInTypes.clear();
	s_KeyWords.clear();
}

Token::Token()
{
	*this = sInvalid;
}

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
	mLOC = ref.mLOC;
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

Token::Type Token::GetType() const
{
	return mType;
}

bool Token::IsValid() const
{
	return (mpData != NULL);
}

bool Token::IsEOF() const
{
	return (mNumOfChar == -1);
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

std::string Token::ToStdString() const
{
	char tempString[MAX_TOKEN_LENGTH + 1];
	ToAnsiString(tempString);
	return std::string(tempString);
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
	while (1) {
		const char* beforeSkip = mCurParsingPtr;
		// Eat the white spaces and new line characters.
		//
		while (*mCurParsingPtr == ' ' || *mCurParsingPtr == '\n' || *mCurParsingPtr == '\t') {
			if (*mCurParsingPtr == '\n')
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
					if (*mCurParsingPtr == '*') 
						isLastAsterisk = true;
					if (*mCurParsingPtr == '\n') 
						++mCurParsingLOC;
					++mCurParsingPtr;
				}
				if (*mCurParsingPtr == '\0') {
					errorMsg = "Comments not ended - unexpected end of file.";
					return Token::sEOF;
				}
				else {
					++mCurParsingPtr; // Skip "/"
				}
			}
			else if (*(mCurParsingPtr + 1) == '/') {
				mCurParsingPtr += 2;
				// Go to the end of the line
				while (*mCurParsingPtr != '\0' && *mCurParsingPtr != '\n') 
					++mCurParsingPtr;

				if (*mCurParsingPtr == '\n') {
					mCurParsingLOC++;
					++mCurParsingPtr;
				}
			}
		}

		// Break from this loop since the pointer doesn't move forward
		if (beforeSkip == mCurParsingPtr)
			break;
	}

	Token ret = Token::sInvalid;
	if (*mCurParsingPtr == '\0') 
		return Token::sEOF;  // Reach the end of the file
	
	// Now it is expecting a token.
	//
	if (_isFirstN_Equal(mCurParsingPtr, "++") ||
		_isFirstN_Equal(mCurParsingPtr, "--") ||
		_isFirstN_Equal(mCurParsingPtr, "||") ||
		_isFirstN_Equal(mCurParsingPtr, "&&") ||
		_isFirstN_Equal(mCurParsingPtr, "==") ) {

		ret = Token(mCurParsingPtr, 2, mCurParsingLOC, Token::kBinaryOp);
		mCurParsingPtr += 2;
	}
	else if (_isFirstN_Equal(mCurParsingPtr, "+") ||
			 _isFirstN_Equal(mCurParsingPtr, "-") ||
			 _isFirstN_Equal(mCurParsingPtr, "*") ||
			 _isFirstN_Equal(mCurParsingPtr, "/") ||
			 _isFirstN_Equal(mCurParsingPtr, "|") ||
			 _isFirstN_Equal(mCurParsingPtr, "&") ||
			 _isFirstN_Equal(mCurParsingPtr, "=") ) {

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
		if (!isFirstCharNumber && !_isAlpha(*mCurParsingPtr) && *mCurParsingPtr != '_') {
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
			++mCurParsingPtr;
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
	mCurParsingLOC = 0;
	mRootCodeDomain = NULL;
}

CompilingContext::~CompilingContext()
{
	if (mRootCodeDomain)
		delete mRootCodeDomain;
}

bool CompilingContext::Parse(const char* content)
{
	mBufferedToken.clear();
	mErrorMessages.clear();
	mContentPtr = content;
	mCurParsingPtr = mContentPtr;
	mCurParsingLOC = 1;

	if (mRootCodeDomain)
		delete mRootCodeDomain;

	PushStatusCode(kAlllowStructDef | kAlllowFuncDef);
	mRootCodeDomain = new RootDomain();
	while (ParseSingleExpression(mRootCodeDomain, NULL));

	return IsEOF() && mErrorMessages.empty();
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
			if (!errMsg.empty())
				AddErrorMessage(Token(NULL, 0, mCurParsingLOC, Token::kUnknown), errMsg);
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
		return mErrorMessages.empty() ? Token::sEOF : Token::sInvalid;
}

bool IsBuiltInType(const Token& token, TypeDesc* out_type)
{
	char tempString[MAX_TOKEN_LENGTH];
	token.ToAnsiString(tempString);
	std::hash_map<std::string, TypeDesc>::iterator it = s_BuiltInTypes.find(tempString);
	if (it != s_BuiltInTypes.end()) {
		if (out_type) *out_type = it->second;
		return true;
	}
	else
		return false;
}

bool IsKeyWord(const Token& token, KeyWord* out_key)
{
	char tempString[MAX_TOKEN_LENGTH];
	token.ToAnsiString(tempString);
	std::hash_map<std::string, KeyWord>::iterator it = s_KeyWords.find(tempString);
	if (it != s_KeyWords.end()) {
		if (out_key) *out_key = it->second;
		return true;
	}
	else
		return false;
}

void DataBlock::AddFloat(Float f)
{
	unsigned char* pData = (unsigned char*)&f;
	for (int i = 0; i < sizeof(Float); ++i)
		mData.push_back(pData[i]);
}

void DataBlock::AddFloat2(Float f0, Float f1)
{
	AddFloat(f0);
	AddFloat(f1);
}

void DataBlock::AddFloat3(Float f0, Float f1, Float f2)
{
	AddFloat2(f0, f1);
	AddFloat(f2);
}

void DataBlock::AddFloat4(Float f0, Float f1, Float f2, Float f3)
{
	AddFloat3(f0, f1, f2);
	AddFloat(f3);
}

void DataBlock::AddInt(int i)
{
	unsigned char* pData = (unsigned char*)&i;
	for (int j = 0; j < sizeof(int); ++j)
		mData.push_back(pData[j]);
}

void DataBlock::AddInt2(int i0, int i1)
{
	AddInt(i0);
	AddInt(i1);
}

void DataBlock::AddInt3(int i0, int i1, int i2)
{
	AddInt2(i0, i1);
	AddInt(i2);
}

void DataBlock::AddInt4(int i0, int i1, int i2, int i3)
{
	AddInt3(i0, i1, i2);
	AddInt(i3);
}

void* DataBlock::GetData()
{
	if (mData.empty())
		return NULL;
	else
		return &mData[0];
}

int DataBlock::GetSize()
{
	return (int)mData.size();
}

Exp_StructDef::Exp_StructDef(std::string name, CodeDomain* parentDomain) :
	CodeDomain(parentDomain)
{
	mStructName = name;
}

Exp_StructDef::~Exp_StructDef()
{

}

Exp_StructDef* Exp_StructDef::Parse(CompilingContext& context, CodeDomain* curDomain)
{
	Token curT = context.GetNextToken();
	if (!curT.IsValid() || !curT.IsEqual("struct")) {
		context.AddErrorMessage(curT, "Structure definition is not started with keyword \"struct\"");
		return NULL;
	}

	curT = context.GetNextToken();
	if (!curT.IsValid() || curT.GetType() != Token::kIdentifier) {
		context.AddErrorMessage(curT, "Invalid structure name.");
		return NULL;
	}

	if (IsBuiltInType(curT) || IsKeyWord(curT) || curDomain->IsTypeDefined(curT.ToStdString()) || curDomain->IsVariableDefined(curT.ToStdString(), true)) {
		context.AddErrorMessage(curT, "Structure name cannot be the built-in type, keyword, user-defined type or previous defined variable name.");
		return NULL;
	}
	std::string structName = curT.ToStdString();

	curT = context.GetNextToken();
	if (!curT.IsValid() || !curT.IsEqual("{")) {
		context.AddErrorMessage(curT, "\"{\" is expected after defining a structure.");
		return NULL;
	}

	bool succeed = false;
	std::auto_ptr<Exp_StructDef> pStructDef(new Exp_StructDef(structName, curDomain));

	context.PushStatusCode(CompilingContext::kAllowVarDef);
	if (context.ParseCodeDomain(pStructDef.get(), "}"))
		succeed = true;
	context.PopStatusCode();

	curT = context.PeekNextToken(0);
	if (curT.IsEqual("}"))
		context.GetNextToken(); // eat the "}"
	else {
		context.AddErrorMessage(curT, "\"}\" is expected.");
		return NULL;
	}

	if (!succeed || pStructDef->GetElementCount() == 0) {
		
		return NULL;
	}
	else {
		curT = context.GetNextToken();
		if (!curT.IsEqual(";")) {
			context.AddErrorMessage(curT, "\";\" is expected.");
			return NULL;
		}
		return pStructDef.release();
	}
}

int Exp_StructDef::GetStructSize() const
{
	int totalSize = 0;
	std::hash_map<std::string, Exp_VarDef*>::const_iterator it = mDefinedVariables.begin();
	for (; it != mDefinedVariables.end(); ++it) {
		int curSize = 0;
		Exp_VarDef* pVarDef = it->second;
		if (pVarDef->GetVarType() == VarType::kStructure) 
			curSize = pVarDef->GetStructDef()->GetStructSize();
		else
			curSize = TypeSize(pVarDef->GetVarType());

		totalSize += curSize;
	}

	return totalSize;
}

int Exp_StructDef::GetElementCount() const
{
	return (int)mDefinedVariables.size();
}

const std::string& Exp_StructDef::GetStructureName() const
{
	return mStructName;
}

VarType Exp_StructDef::GetElementType(int idx, const Exp_StructDef* &outStructDef) const
{
	std::hash_map<int, Exp_VarDef*>::const_iterator it = mIdx2ValueDefs.find(idx);
	if (it == mIdx2ValueDefs.end())
		return VarType::kInvalid;
	else {
		VarType ret = it->second->GetVarType();
		outStructDef = it->second->GetStructDef();
		return ret;
	}
}

void CompilingContext::AddErrorMessage(const Token& token, const std::string& str)
{
	mErrorMessages.push_back(std::pair<Token, std::string>(token, str));
}

bool CompilingContext::IsStructDefinePartten()
{
	Token t0 = PeekNextToken(0);
	Token t1 = PeekNextToken(1);
	Token t2 = PeekNextToken(2);

	if (!t0.IsValid() || !t1.IsValid() || !t2.IsValid())
		return false;

	if (!t0.IsEqual("struct"))
		return false;


	if (t1.GetType() != Token::kIdentifier)
		return false;

	if (!t2.IsEqual("{"))
		return false;

	return true;
}

bool CompilingContext::IsFunctionDefinePartten()
{
	Token t0 = PeekNextToken(0);
	Token t1 = PeekNextToken(1);
	Token t2 = PeekNextToken(2);

	if (!t0.IsValid() || !t1.IsValid() || !t2.IsValid())
		return false;

	if (t0.GetType() != Token::kIdentifier || IsKeyWord(t0))
		return false;

	if (t1.GetType() != Token::kIdentifier)
		return false;

	if (!t2.IsEqual("("))
		return false;

	return true;
}


bool CompilingContext::IsVarDefinePartten(bool allowInit)
{
	Token t0 = PeekNextToken(0);
	Token t1 = PeekNextToken(1);

	if (!t0.IsValid() || !t1.IsValid())
		return false;

	if (t0.GetType() != Token::kIdentifier || IsKeyWord(t0))
		return false;

	if (t1.GetType() != Token::kIdentifier)
		return false;

	return true;
}

bool Exp_VarDef::Parse(CompilingContext& context, CodeDomain* curDomain, std::vector<Exp_VarDef*>& out_defs)
{
	Token curT = context.GetNextToken();
	TypeDesc typeDesc;
	out_defs.clear();

	if (!curT.IsValid() || 
		(!IsBuiltInType(curT, &typeDesc) &&
		!curDomain->IsTypeDefined(curT.ToStdString()))) {
		context.AddErrorMessage(curT, "Invalid token, must be a valid built-in type of user-defined type.");
		return false;
	}

	VarType varType = typeDesc.type;
	Exp_StructDef* pStructDef = NULL;
	if (varType == VarType::kInvalid) {
		pStructDef = curDomain->GetStructDefineByName(curT.ToStdString());
		varType = VarType::kStructure;
	}

	
	std::list<std::auto_ptr<Exp_VarDef> > tempOutDefs;
	bool bContinue = false;;
	do {
		curT = context.GetNextToken();
		if (curT.GetType() != Token::kIdentifier) {
			context.AddErrorMessage(curT, "Invalid token, must be a valid identifier.");
			return false;
		}
		if (IsBuiltInType(curT)) {
			context.AddErrorMessage(curT, "The built-in type cannot be used as variable.");
			return false;
		}
		if (IsKeyWord(curT)) {
			context.AddErrorMessage(curT, "The keyword cannot be used as variable.");
			return false;
		}
		if (curDomain->IsTypeDefined(curT.ToStdString())) {
			context.AddErrorMessage(curT, "A user-defined type cannot be redefined as variable.");
			return false;
		}

		if (curDomain->IsVariableDefined(curT.ToStdString(), false)) {
			context.AddErrorMessage(curT, "Variable redefination is not allowed in the same code block.");
			return false;
		}

		// Test if the coming token is "=", which indicates the variable initialization.
		//
		Exp_ValueEval* pInitValue = NULL;
		if (context.PeekNextToken(0).IsEqual("=")) {

			if (context.GetStatusCode() & CompilingContext::kAllowVarInit) {
				context.GetNextToken(); // Eat the "="
				// Handle the variable initialization
				pInitValue = context.ParseComplexExpression(curDomain, ";");
				if (!pInitValue)
					return false;
			}
			else {
				context.AddErrorMessage(curT, "Variable initialization not allowed in this domain.");
				return false;
			}
		}

		Exp_VarDef* ret = new Exp_VarDef(varType, curT, pInitValue);
		if (varType == VarType::kStructure)
			ret->SetStructDef(pStructDef);

		// Now we can accept this variable
		//
		tempOutDefs.push_back(std::auto_ptr<Exp_VarDef>(ret));

		// Test is the next token is ";", which indicates the end of the variable definition expression.
		//
		if (context.PeekNextToken(0).IsEqual(";")) {
			break;
		}

		bContinue = context.PeekNextToken(0).IsEqual(",");
		if (bContinue) context.GetNextToken(); // eat the ","
	} while (bContinue);

	if (context.PeekNextToken(0).IsEqual(";")) {
		context.GetNextToken(); // eat the ";"
				
		if (!tempOutDefs.empty()) {
			for (std::list<std::auto_ptr<Exp_VarDef> >::iterator it = tempOutDefs.begin(); it != tempOutDefs.end(); ++it) {
				out_defs.push_back((*it).release());
			}
			return true;
		}
		else
			return false;
	}
	else 
		return false;
}

bool CompilingContext::IsEOF() const
{
	return (*mCurParsingPtr == '\0');
}

void CompilingContext::PushStatusCode(int code)
{
	mStatusCode.push_back(code);
}

int CompilingContext::GetStatusCode()
{
	if (mStatusCode.empty())
		return 0;
	else
		return mStatusCode.back();
}

void CompilingContext::PopStatusCode()
{
	mStatusCode.pop_back();
}

bool CompilingContext::ParseSingleExpression(CodeDomain* curDomain, const char* endT)
{
	if (PeekNextToken(0).IsEOF())
		return false;
	
	if (endT && PeekNextToken(0).IsEqual(endT))
		return false;
	else if ((GetStatusCode() & kAlllowFuncDef) && IsFunctionDefinePartten()) {
		// Parse the function declaration.
		if (!Exp_FunctionDecl::Parse(*this, curDomain))
			return false;
	}
	else if ((GetStatusCode() & kAllowVarDef) && IsVarDefinePartten(true)) {
		std::vector<Exp_VarDef*>  varDefs;
		if (Exp_VarDef::Parse(*this, curDomain, varDefs)) {
			for (int i = 0; i < (int)varDefs.size(); ++i)
				curDomain->AddVarDefExpression(varDefs[i]);
		}
		else
			return false;
	}
	else if ((GetStatusCode() & kAlllowStructDef) && IsStructDefinePartten()) {
		Exp_StructDef* structDef = Exp_StructDef::Parse(*this, curDomain);
		if (structDef)
			curDomain->AddStructDefExpression(structDef);
		else
			return false;
	}
	else if (GetStatusCode() & kAllowValueExp) {

		Exp_ValueEval* pNewExp = NULL;
		if (PeekNextToken(0).IsEqual("return")) {
			GetNextToken(); // Eat the "return"
			Exp_ValueEval* pValue = NULL;
			if (!PeekNextToken(0).IsEqual(";")) {
				pValue = ParseComplexExpression(curDomain, ";");
				if (!pValue) return false;
				GetNextToken(); // Eat the ";"
			}
			pNewExp = new Exp_FuncRet(pValue);
		}
		else {
			// Try to parse a complex expression
			pNewExp = ParseComplexExpression(curDomain, ";");
			if (!pNewExp) {
				Token curT = PeekNextToken(0);
				AddErrorMessage(curT, "Unexpected end of file");
				return false;
			}
			GetNextToken(); // Eat the ";"
		}

		if (pNewExp)
			curDomain->AddValueExpression(pNewExp);
	}
	else {
		if (!PeekNextToken(0).IsEOF()) {
			AddErrorMessage(PeekNextToken(0), "Unexpected expression in current domain.");
			return false;
		}
		else
			return false;
	}

	return true;
}

bool CompilingContext::ParseCodeDomain(CodeDomain* curDomain, const char* endT)
{
	if (IsEOF())
		return false;

	bool multiExp = PeekNextToken(0).IsEqual("{");

	if (multiExp) {
		GetNextToken(); // eat the "{"

		CodeDomain* childDomain = new CodeDomain(curDomain);
		curDomain->AddDomainExpression(childDomain);
		if (!ParseCodeDomain(childDomain, "}"))
			return false;

		Token endT = PeekNextToken(0);
		if (endT.IsEqual("}"))
			GetNextToken(); // eat the "}"
		else {
			AddErrorMessage(endT, "\"}\" is expected.");
			return false;
		}
	}
	else {
		while (ParseSingleExpression(curDomain, endT));
		if (!mErrorMessages.empty())
			return false;
	}

	return true;
}

CodeDomain::CodeDomain(CodeDomain* parent)
{
	mpParentDomain = parent;
}

CodeDomain::~CodeDomain()
{

	std::vector<Expression*>::iterator it_exp = mExpressions.begin();
	for (; it_exp != mExpressions.end(); ++it_exp) {
		delete *it_exp;
	}
}

void CodeDomain::AddValueExpression(Exp_ValueEval* exp)
{
	if (exp) {
		mExpressions.push_back(exp);
	}
}

void CodeDomain::AddStructDefExpression(Exp_StructDef* exp)
{
	if (exp) {
		mExpressions.push_back(exp);
		mDefinedStructures[exp->GetStructureName()] = exp;
	}
}

void CodeDomain::AddVarDefExpression(Exp_VarDef* exp)
{
	if (exp) {
		mExpressions.push_back(exp);
		mDefinedVariables[exp->GetVarName().ToStdString()] = exp;
	}
}

void CodeDomain::AddFunctionDefExpression(Exp_FunctionDecl* exp)
{
	if (exp) {
		mExpressions.push_back(exp);
		mDefinedFunctions[exp->GetFunctionName()] = exp;
	}
}

void CodeDomain::AddDomainExpression(CodeDomain* exp)
{
	if (exp) {
		mExpressions.push_back(exp);
	}
}

void CodeDomain::AddDefinedType(Exp_StructDef* pStructDef)
{
	if (pStructDef)
		mDefinedStructures[pStructDef->GetStructureName()] = pStructDef;
}

bool CodeDomain::IsTypeDefined(const std::string& typeName)
{
	if (mDefinedStructures.find(typeName) == mDefinedStructures.end()) 
		return mpParentDomain ? mpParentDomain->IsTypeDefined(typeName) : false;
	else
		return true;
}

void CodeDomain::AddDefinedVariable(const Token& t, Exp_VarDef* pDef)
{
	mDefinedVariables[t.ToStdString()] = pDef;
}

bool CodeDomain::IsVariableDefined(const std::string& varName, bool includeParent)
{
	if (mDefinedVariables.find(varName) == mDefinedVariables.end()) {
		if (includeParent && mpParentDomain) 
			return mpParentDomain->IsVariableDefined(varName, includeParent);
		else
			return false;
	}
	else
		return true;
}

void CodeDomain::AddDefinedFunction(Exp_FunctionDecl* pFunc)
{
	std::string funcName = pFunc->GetFunctionName();
	std::hash_map<std::string, Exp_FunctionDecl*>::iterator it = mDefinedFunctions.find(funcName);
	if (it == mDefinedFunctions.end())
		mDefinedFunctions[funcName] = pFunc;
	else
		assert(0); 
}

bool CodeDomain::IsFunctionDefined(const std::string& funcName)
{
	std::hash_map<std::string, Exp_FunctionDecl*>::iterator it = mDefinedFunctions.find(funcName);
	return (it != mDefinedFunctions.end());
}

Exp_StructDef* CodeDomain::GetStructDefineByName(const std::string& structName)
{
	std::hash_map<std::string, Exp_StructDef*>::iterator it = mDefinedStructures.find(structName);
	if (it != mDefinedStructures.end())
		return it->second;
	else 
		return mpParentDomain->GetStructDefineByName(structName);
}

Exp_VarDef* CodeDomain::GetVarDefExpByName(const std::string& varName)
{
	std::hash_map<std::string, Exp_VarDef*>::iterator it = mDefinedVariables.find(varName);
	if (it != mDefinedVariables.end())
		return it->second;
	else 
		return mpParentDomain->GetVarDefExpByName(varName);
}

Exp_VarDef::Exp_VarDef(VarType type, const Token& var, Exp_ValueEval* pInitValue)
{
	mVarType = type;
	mpStructDef = NULL;
	mVarName = var;
	mpInitValue = pInitValue;
}

Exp_VarDef::~Exp_VarDef()
{
	if (mpInitValue)
		delete mpInitValue;
}

void Exp_VarDef::SetStructDef(Exp_StructDef* pStruct)
{
	mpStructDef = pStruct;
}

Exp_ValueEval* Exp_VarDef::GetVarInitExp()
{
	return mpInitValue;
}

Token Exp_VarDef::GetVarName() const
{
	return mVarName;
}

VarType Exp_VarDef::GetVarType() const
{
	return mVarType;
}

Exp_StructDef* Exp_VarDef::GetStructDef()
{
	return mpStructDef;
}

RootDomain::RootDomain() :
	CodeDomain(NULL)
{

}

RootDomain::~RootDomain()
{

}

Exp_BinaryOp::Exp_BinaryOp(const std::string& op, Exp_ValueEval* pLeft, Exp_ValueEval* pRight)
{
	mOperator = op;
	mpLeftExp = pLeft;
	mpRightExp = pRight;
}

Exp_BinaryOp::~Exp_BinaryOp()
{
	delete mpLeftExp;
	delete mpRightExp;
}

bool CompilingContext::ExpectAndEat(const char* str)
{
	Token curT = GetNextToken();
	if (curT.IsEqual(str)) 
		return true;
	else {
		std::string errMsg = str;
		errMsg += " is expected.";
		AddErrorMessage(curT, errMsg);
		return false;
	}
}

bool CompilingContext::ExpectTypeAndEat(CodeDomain* curDomain, VarType& outType, Exp_StructDef*& outStructDef)
{
	TypeDesc retType;
	Token curT = GetNextToken();
	if (!IsBuiltInType(curT, &retType) && !curDomain->IsTypeDefined(curT.ToStdString())) {
		AddErrorMessage(curT, "Expect built-in type or a predefined structure.");
		return false;
	}
	outType = retType.type;
	outStructDef = NULL;
	if (retType.type == VarType::kInvalid) {
		outType = VarType::kStructure;
		outStructDef = curDomain->GetStructDefineByName(curT.ToStdString());
	}
	return true;
}

Exp_ValueEval* CompilingContext::ParseSimpleExpression(CodeDomain* curDomain)
{
	Token curT = GetNextToken();
	if (curT.GetType() != Token::kIdentifier && 
		curT.GetType() != Token::kUnaryOp && 
		curT.GetType() != Token::kConstFloat &&
		curT.GetType() != Token::kConstInt &&
		curT.GetType() != Token::kOpenParenthesis) {
		AddErrorMessage(curT, "Invalid token for value expression.");
		return NULL;
	}

	std::auto_ptr<Exp_ValueEval> result;

	if (curT.GetType() == Token::kIdentifier) {
		// This identifier must be a already defined variable, built-in type or constant
		TypeDesc tpDesc;
		if (IsBuiltInType(curT, &tpDesc)) {
			// Parse and return the built-in type initialize
			if (!ExpectAndEat("(")) return NULL;

			std::auto_ptr<Exp_ValueEval> exp[4];
			
			bool succeed = false;
			Token lastT;
			for (int i = 0; i < tpDesc.elemCnt; ++i) {
				exp[i].reset(ParseComplexExpression(curDomain, i == (tpDesc.elemCnt - 1) ? ")" : ","));
				if (!exp[i].get()) return NULL;
				GetNextToken(); // Eat the end token(")" or ",")
			}

			Exp_ValueEval* expArray[4] = {exp[0].get(), exp[1].get(), exp[2].get(), exp[3].get()};
			for (int i = 0; i < tpDesc.elemCnt; ++i)
				exp[i].release();
			result.reset(new Exp_BuiltInInitializer(expArray, tpDesc.elemCnt, tpDesc.type));
		}
		else if (curDomain->IsVariableDefined(curT.ToStdString(), true)) {
			// Return a value ref expression
			result.reset(new Exp_VariableRef(curT, curDomain->GetVarDefExpByName(curT.ToStdString())));
		}
		else if (curT.IsEqual("true") ||
				 curT.IsEqual("false")) {
			bool value = curT.IsEqual("true"); // Eat the "true" or "false"
			result.reset(new Exp_TrueOrFalse(value));
		}
		else {
			// TODO: if the identifier is a function name, it should return the function call expression
		}
	}
	else if (curT.GetType() == Token::kUnaryOp) {
		// Generate a unary operator expression
		Exp_ValueEval* ret = ParseSimpleExpression(curDomain);
		if (ret)
			result.reset(new Exp_UnaryOp(curT.ToStdString(), ret));
	}
	else if (curT.GetType() == Token::kOpenParenthesis) {
		// This expression starts with "(", so it needs to end up with ")".
		Exp_ValueEval* ret = ParseComplexExpression(curDomain, ")");
		if (ret) {
			result.reset(ret);
			GetNextToken(); // Eat the ")"
		}
	}
	else {
		// return a constant expression
		if (curT.GetType() != Token::kConstFloat && curT.GetType() != Token::kConstInt) {
			AddErrorMessage(curT, "Unexpected token.");
			return NULL;
		}

		result.reset(new Exp_Constant(curT.GetConstValue(), curT.GetType() == Token::kConstFloat));
	}

	if (!result.get()) {
		AddErrorMessage(curT, "Unexpected token.");
		return NULL;
	}
	// This is not the end of simple expression, I need to check for the next token to see if it has swizzle or structure member access.
	// e.g. myVar.xyz, myVar.myVar
	while (PeekNextToken(0).IsEqual(".")) {
		// Need to deal with dot operator
		GetNextToken();  // Eat the "."
		curT = GetNextToken();
		if (curT.GetType() != Token::kIdentifier) {
			AddErrorMessage(curT, "Unexpected token, identifier is expected.");
			return NULL;
		}

		result.reset(new Exp_DotOp(curT.ToStdString(), result.release()));
	}

	return result.release();
}

Exp_ValueEval* CompilingContext::ParseComplexExpression(CodeDomain* curDomain, const char* pEndToken)
{
	Exp_ValueEval* simpleExp0 = ParseSimpleExpression(curDomain);
	if (!simpleExp0) {
		// Must have some error message if it failed to parse a simple expression
		assert(!mErrorMessages.empty()); 
		return NULL;
	}

	Token curT = PeekNextToken(0);
	if (curT.IsEqual(pEndToken)) {
		return simpleExp0;
	}
	if (curT.GetType() != Token::kBinaryOp) {
		AddErrorMessage(curT, "Expect a binary operator.");
		return NULL;
	}
	GetNextToken(); // Eat the binary operator

	int op0_level = curT.GetBinaryOpLevel();
	std::string op0_str = curT.ToStdString();

	Exp_ValueEval* simpleExp1 = ParseSimpleExpression(curDomain);
	if (!simpleExp1) {
		// Must have some error message if it failed to parse a simple expression
		assert(!mErrorMessages.empty()); 
		return NULL;
	}

	Token nextT = PeekNextToken(0);
	// Get the next token to decide if the next binary operator is in high level of priority
	if (nextT.IsEqual(pEndToken)) {
		// we are done for this complex value expression, return it.
		Exp_BinaryOp* pBinaryOp = new Exp_BinaryOp(op0_str, simpleExp0, simpleExp1);
		return pBinaryOp;
	}
	else if (nextT.GetType() == Token::kBinaryOp) {

		GetNextToken(); // Eat the binary operator
		int op1_level = nextT.GetBinaryOpLevel();
		std::string op1_str = nextT.ToStdString();

		if (op1_level > op0_level) {
			Exp_ValueEval* simpleExp2 = ParseComplexExpression(curDomain, pEndToken);
			if (simpleExp2) {
				Exp_BinaryOp* pBinaryOp1 = new Exp_BinaryOp(op1_str, simpleExp1, simpleExp2);
				Exp_BinaryOp* pBinaryOp0 = new Exp_BinaryOp(op0_str, simpleExp0, pBinaryOp1);
				return pBinaryOp0;
			}
		}
		else {
			Exp_ValueEval* simpleExp2 = ParseComplexExpression(curDomain, pEndToken);
			if (simpleExp2) {
				Exp_BinaryOp* pBinaryOp0 = new Exp_BinaryOp(op1_str, simpleExp0, simpleExp1);
				Exp_BinaryOp* pBinaryOp1 = new Exp_BinaryOp(op0_str, pBinaryOp0, simpleExp2);
				return pBinaryOp1;
			}
		}
	}

	return NULL;
}

Exp_Constant::Exp_Constant(double v, bool f)
{
	mValue = v;
	mIsFromFloat = f;
}

Exp_Constant::~Exp_Constant()
{

}

double Exp_Constant::GetValue() const
{
	return mValue;
}

bool Exp_Constant::CheckSemantic(TypeInfo& outType, std::string& errMsg, std::vector<std::string>& warnMsg)
{
	outType.type = mIsFromFloat ? VarType::kFloat : VarType::kInt;
	outType.pStructDef = NULL;
	return true;
}

Exp_VariableRef::Exp_VariableRef(Token t, Exp_VarDef* pDef)
{
	mVariable = t;
	mpDef = pDef;
}

Exp_VariableRef::~Exp_VariableRef()
{

}

bool Exp_VariableRef::CheckSemantic(TypeInfo& outType, std::string& errMsg, std::vector<std::string>& warnMsg)
{
	outType.type = mpDef->GetVarType();
	outType.pStructDef = mpDef->GetStructDef();
	return true;
}

Exp_StructDef* Exp_VariableRef::GetStructDef()
{
	if (mpDef) 
		return mpDef->GetStructDef();
	else
		return NULL;
}


Exp_BuiltInInitializer::Exp_BuiltInInitializer(Exp_ValueEval** pExp, int cnt, VarType tp)
{
	mType = tp;
	mpSubExprs[0] = NULL;
	mpSubExprs[1] = NULL;
	mpSubExprs[2] = NULL;
	mpSubExprs[3] = NULL;

	int aCnt = cnt > 4 ? 4 : cnt;
	for (int i = 0; i < aCnt; ++i) {
		mpSubExprs[i] = pExp[i];
	}
}

Exp_BuiltInInitializer::~Exp_BuiltInInitializer()
{
	for (int i = 0; i < 4; ++i) {
		if (mpSubExprs[i])
			delete mpSubExprs[i];
	}
}

bool Exp_BuiltInInitializer::CheckSemantic(TypeInfo& outType, std::string& errMsg, std::vector<std::string>& warnMsg)
{
	int ElemCnt = TypeElementCnt(mType);
	int ElemCntGiven = 0;
	bool hasFloat = false;
	bool SubExpCnt = 0;
	for (int i = 0; i < 4; ++i) {
		Exp_ValueEval* curExp = mpSubExprs[i];
		if (curExp) {
			TypeInfo typeInfo;
			if (!curExp->CheckSemantic(typeInfo, errMsg, warnMsg)) 
				return false;
			if (!IsBuiltInType(typeInfo.type)) {
				errMsg = "Must be built-in type.";
				return false;
			}
			if (!IsIntegerType(typeInfo.type))
				hasFloat = true;
			ElemCntGiven += TypeElementCnt(typeInfo.type);
		}
	}

	if (ElemCntGiven != ElemCnt) {
		errMsg = "Bad built-in type initialization.";
		return false;
	}

	if (IsIntegerType(mType)) {
		if (hasFloat) warnMsg.push_back("Integer implicitly converted to float type.");
	}
	outType.type = mType;
	outType.pStructDef = NULL;
	return true;
}

Exp_UnaryOp::Exp_UnaryOp(const std::string& op, Exp_ValueEval* pExp)
{
	mOpType = op;
	mpExpr = pExp;
}

Exp_UnaryOp::~Exp_UnaryOp()
{
	if (mpExpr)
		delete mpExpr;
}

bool Exp_UnaryOp::CheckSemantic(TypeInfo& outType, std::string& errMsg, std::vector<std::string>& warnMsg)
{
	// will unary operation change the value type?
	return mpExpr->CheckSemantic(outType, errMsg, warnMsg);
}


bool Exp_BinaryOp::CheckSemantic(TypeInfo& outType, std::string& errMsg, std::vector<std::string>& warnMsg)
{
	TypeInfo leftType, rightType;
	if (!mpLeftExp->CheckSemantic(leftType, errMsg, warnMsg) || !mpLeftExp->CheckSemantic(rightType, errMsg, warnMsg))
		return false;
	if (leftType.type == VarType::kStructure || rightType.type == VarType::kStructure) {
		if (mOperator == "=") {
			if (leftType.pStructDef != rightType.pStructDef) {
				errMsg = "Cannot assign from different structure.";
				return false;
			}
			else {
				outType.pStructDef = leftType.pStructDef;
				outType.type = VarType::kStructure;
				return true;
			}
		}
		else {
			errMsg = "Cannot do binary operation between structure or built-in types.";
			return false;
		}
	}
	else {

		if (leftType.type != rightType.type) {
			if (TypeElementCnt(leftType.type) > TypeElementCnt(rightType.type)) {
				errMsg = "Cannot do binary operation with right argument of less elements.";
				return false;
			}
			if (IsIntegerType(leftType.type) && !IsIntegerType(rightType.type))
				warnMsg.push_back("Integer implicitly converted to float type.");
		}
		outType.pStructDef = NULL;
		outType.type = leftType.type;
		return true;
	}
}

Exp_DotOp::Exp_DotOp(const std::string& opStr, Exp_ValueEval* pExp)
{
	mOpStr = opStr;
	mpExp = pExp;
}

Exp_DotOp::~Exp_DotOp()
{
	if (mpExp)
		delete mpExp;
}

bool Exp_DotOp::CheckSemantic(TypeInfo& outType, std::string& errMsg, std::vector<std::string>& warnMsg)
{
	TypeInfo parentType;
	if (!mpExp->CheckSemantic(parentType, errMsg, warnMsg))
		return false;
	if (parentType.type == VarType::kStructure) {
		if (parentType.pStructDef->IsVariableDefined(mOpStr, false)) {
			Exp_VarDef* pDef = parentType.pStructDef->GetVarDefExpByName(mOpStr);
			assert(pDef);
			outType.type = pDef->GetVarType();
			if (outType.type == VarType::kStructure)
				outType.pStructDef = pDef->GetStructDef();
			else
				outType.pStructDef = NULL;
			return true;
		}
		else {
			errMsg = "Invalid structure member name.";
			return false;
		}
	}
	else {
		// TODO: The dotOp is the swizzle operation.
		int swizzleIdx[4];
		int elemCnt = ConvertSwizzle(mOpStr.c_str(), swizzleIdx);
		if (elemCnt <= 0) {
			errMsg = "Invalid swizzle expression.";
			return false;
		}

		outType.type = MakeType(IsIntegerType(parentType.type), elemCnt);
		outType.pStructDef = NULL;
		return false;

	}
}

Exp_FunctionDecl::Exp_FunctionDecl(CodeDomain* parent) :
	CodeDomain(parent)
{
	mReturnType = VarType::kInvalid;
	mpRetStruct = NULL;
}

Exp_FunctionDecl::~Exp_FunctionDecl()
{

}

const std::string Exp_FunctionDecl::GetFunctionName() const
{
	return mFuncName;
}

int Exp_FunctionDecl::GetArgumentCnt() const
{
	return (int)mArgments.size();
}

Exp_FunctionDecl::ArgDesc* Exp_FunctionDecl::GetArgumentDesc(int idx)
{
	return &mArgments[idx];
}

bool Exp_FunctionDecl::HasSamePrototype(const Exp_FunctionDecl& ref) const
{
	if (mFuncName != ref.mFuncName)
		return false;
	if (mReturnType != ref.mReturnType)
		return false;
	if (mpRetStruct != ref.mpRetStruct)
		return false;
	if (mArgments.size() != ref.mArgments.size())
		return false;
	for (int i = 0; i < (int)mArgments.size(); ++i) {
		if (mArgments[i].isByRef != ref.mArgments[i].isByRef ||
			mArgments[i].type != ref.mArgments[i].type ||
			mArgments[i].pStructDef != ref.mArgments[i].pStructDef ||
			mArgments[i].token.IsEqual(ref.mArgments[i].token) )
			return NULL;
	}
	return true;
}

Exp_FunctionDecl* CodeDomain::GetFunctionDeclByName(const std::string& funcName)
{
	std::hash_map<std::string, Exp_FunctionDecl*>::iterator it = mDefinedFunctions.find(funcName);
	if (it != mDefinedFunctions.end())
		return it->second;
	return NULL;
}

bool Exp_FunctionDecl::Parse(CompilingContext& context, CodeDomain* curDomain)
{
	std::auto_ptr<Exp_FunctionDecl> result(new Exp_FunctionDecl(curDomain));
	// The first token needs to be the returned type of the function
	//
	if (!context.ExpectTypeAndEat(curDomain, result->mReturnType, result->mpRetStruct))
		return false;

	// The second token should be the function name
	//
	Token curT = context.GetNextToken();
	bool alreadyDefined = (curDomain->IsFunctionDefined(curT.ToStdString()));
	result->mFuncName = curT.ToStdString();

	// The coming tokens should be the function arguments in a pair of brackets
	// e.g. (Type0 arg0, Type1 arg1)
	//
	if (!context.ExpectAndEat("("))
		return false;
	do {
		Exp_FunctionDecl::ArgDesc argDesc;
		if (!context.ExpectTypeAndEat(curDomain, argDesc.type, argDesc.pStructDef))
			return false;
		Token argT = context.GetNextToken();
		for (int i = 0; i < (int)result->mArgments.size(); ++i) {
			if (argT.IsEqual(result->mArgments[i].token)) {
				context.AddErrorMessage(curT, "Function argument redefined.");
				return false;
			}
		}
		argDesc.token = argT;
		argDesc.isByRef = false; // TODO: handle passing-by-reference argument
		result->mArgments.push_back(argDesc);

		if (context.PeekNextToken(0).IsEqual(","))
			context.GetNextToken(); // Eat the ","

	} while (!context.PeekNextToken(0).IsEqual(")"));

	context.GetNextToken(); // Eat the ")"

	Exp_FunctionDecl* pFuncDef = NULL;
	if (alreadyDefined) {
		Exp_FunctionDecl* pFuncDef = curDomain->GetFunctionDeclByName(result->mFuncName);
		assert(pFuncDef);
		if (!result->HasSamePrototype(*pFuncDef)) {
			context.AddErrorMessage(curT, "Function declared with different prototype.");
			return false;
		}
		result.reset(NULL);
	}
	else {
		pFuncDef = result.get();
		curDomain->AddFunctionDefExpression(result.release());
	}

	bool ret = true;
	if (context.PeekNextToken(0).IsEqual("{")) {
		// The function body is expected, if there is already a function body for this function, kick it out.
		if (alreadyDefined) {
			if (!pFuncDef->mExpressions.empty()) {
				context.AddErrorMessage(curT, "Function already has a body.");
				return false;
			}
		}

		for (int i = 0; i < (int)pFuncDef->mArgments.size(); ++i) {
			Exp_VarDef* pExp = new Exp_VarDef(pFuncDef->mArgments[i].type, pFuncDef->mArgments[i].token, NULL);
			if (pFuncDef->mArgments[i].type == VarType::kStructure)
				pExp->SetStructDef(pFuncDef->mArgments[i].pStructDef);
			pFuncDef->AddVarDefExpression(pExp);
		}

		// Now parse the function body
		context.PushStatusCode(CompilingContext::kAlllowStructDef | CompilingContext::kAllowReturnExp | CompilingContext::kAllowValueExp | CompilingContext::kAllowVarDef | CompilingContext::kAllowVarInit);
		if (!context.ParseCodeDomain(pFuncDef, NULL))
			ret = false;
		context.PopStatusCode();

	}

	return ret;
}

Exp_FuncRet::Exp_FuncRet(Exp_ValueEval* pRet)
{
	mpRetValue = pRet;
}

Exp_FuncRet::~Exp_FuncRet()
{
	if (mpRetValue)
		delete mpRetValue;
}

bool Exp_FuncRet::CheckSemantic(TypeInfo& outType, std::string& errMsg, std::vector<std::string>& warnMsg)
{
	if (mpRetValue)
		return mpRetValue->CheckSemantic(outType, errMsg, warnMsg);
	else {
		outType.type = VarType::kVoid;
		outType.pStructDef = NULL;
		return true;
	}
}

Exp_TrueOrFalse::Exp_TrueOrFalse(bool value)
{
	mValue = value;
}

Exp_TrueOrFalse::~Exp_TrueOrFalse()
{

}

bool Exp_TrueOrFalse::GetValue() const
{
	return mValue;
}

bool Exp_TrueOrFalse::CheckSemantic(TypeInfo& outType, std::string& errMsg, std::vector<std::string>& warnMsg)
{
	outType.type = VarType::kBoolean;
	outType.pStructDef = NULL;
	return true;
}

} // namespace SC