#include "parser_AST_Gen.h"
#include <stdio.h>
#include <assert.h>

namespace SC {

Token Token::sInvalid = Token(NULL, 0, 0, Token::kUnknown);
Token Token::sEOF = Token(NULL, -1, -1, Token::kUnknown);

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

	mRootCodeDomain = new RootDomain();
	while (ParseCodeDomain(mRootCodeDomain));

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

struct TypeDesc {
	VarType type;
	int elemCnt;
	bool isInt;

	TypeDesc() {type = VarType::kInvalid; elemCnt = 0; isInt = false;}
	TypeDesc(VarType tp, int cnt, bool i) {type = tp; elemCnt = cnt; isInt = i;}
};

static std::hash_map<std::string, TypeDesc> s_BuiltInTypes;
static std::hash_map<std::string, KeyWord> s_KeyWords;

void Initialize_AST_Gen()
{
	s_BuiltInTypes["float"] = TypeDesc(kFloat, 1, false);
	s_BuiltInTypes["float2"] = TypeDesc(kFloat2, 1, false);
	s_BuiltInTypes["float3"] = TypeDesc(kFloat3, 1, false);
	s_BuiltInTypes["float4"] = TypeDesc(kFloat4, 1, false);

	s_BuiltInTypes["int"] = TypeDesc(kInt, 1, true);
	s_BuiltInTypes["int2"] = TypeDesc(kInt2, 1, true);
	s_BuiltInTypes["int3"] = TypeDesc(kInt3, 1, true);
	s_BuiltInTypes["int4"] = TypeDesc(kInt4, 1, true);

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
	mStructDomain(parentDomain)
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

	bool succeed = true;
	Exp_StructDef* pStructDef = new Exp_StructDef(structName, curDomain);
	do {
		std::vector<Exp_VarDef*> varDefs;
		if (!Exp_VarDef::Parse(context, &pStructDef->mStructDomain, false, varDefs)) {
			succeed = false;
			break;
		}
		
		for (int vi = 0; vi < (int)varDefs.size(); ++vi) {
			pStructDef->AddElement(varDefs[vi]->GetVarName().ToStdString(), varDefs[vi]->GetVarType(), varDefs[vi]->GetStructDef());
			delete varDefs[vi];
		}
		varDefs.clear();

		if (context.PeekNextToken(0).IsEqual("}")) {
			context.GetNextToken(); // eat "}"
			break;
		}
	} while (context.PeekNextToken(0).IsValid());


	if (!succeed || pStructDef->GetElementCount() == 0) {
		
		delete pStructDef;
		return NULL;
	}
	else {
		curT = context.GetNextToken();
		if (!curT.IsEqual(";")) {
			context.AddErrorMessage(curT, "\";\" is expected.");
			delete pStructDef;
			return NULL;
		}
		curDomain->AddDefinedType(pStructDef);
		return pStructDef;
	}
}

void Exp_StructDef::AddElement(const std::string& varName, VarType type, Exp_StructDef* pStructDef)
{
	Element elem;
	elem.isStruct = (type == VarType::kStructure);
	elem.name = varName;
	elem.type = (type == VarType::kStructure ? (void*)pStructDef : (void*)type);
	mElements.push_back(elem);
}

int Exp_StructDef::GetStructSize() const
{
	int totalSize = 0;
	for (size_t i = 0; i < mElements.size(); ++i) {
		int curSize = 0;
		if (!mElements[i].isStruct) {
			switch ((VarType)(int)mElements[i].type)
			{
			case VarType::kFloat:
				curSize = sizeof(Float);
				break;
			case VarType::kFloat2:
				curSize = sizeof(Float)*2;
				break;
			case VarType::kFloat3:
				curSize = sizeof(Float)*3;
				break;
			case VarType::kFloat4:
				curSize = sizeof(Float)*4;
				break;
			case VarType::kInt:
				curSize = sizeof(int);
				break;
			case VarType::kInt2:
				curSize = sizeof(int)*2;
				break;
			case VarType::kInt3:
				curSize = sizeof(int)*3;
				break;
			case VarType::kInt4:
				curSize = sizeof(int)*4;
				break;
			}
		}
		else
			curSize = ((Exp_StructDef*)mElements[i].type)->GetStructSize();

		totalSize += curSize;
	}

	return totalSize;
}

int Exp_StructDef::GetElementCount() const
{
	return (int)mElements.size();
}

const std::string& Exp_StructDef::GetStructureName() const
{
	return mStructName;
}

VarType Exp_StructDef::GetElementType(int idx) const
{
	return ((VarType)(int)mElements[idx].type);
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

bool Exp_VarDef::Parse(CompilingContext& context, CodeDomain* curDomain, bool allowInit, std::vector<Exp_VarDef*>& out_defs)
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
			return NULL;
		}
		if (IsBuiltInType(curT)) {
			context.AddErrorMessage(curT, "The built-in type cannot be used as variable.");
			return NULL;
		}
		if (IsKeyWord(curT)) {
			context.AddErrorMessage(curT, "The keyword cannot be used as variable.");
			return NULL;
		}
		if (curDomain->IsTypeDefined(curT.ToStdString())) {
			context.AddErrorMessage(curT, "A user-defined type cannot be redefined as variable.");
			return NULL;
		}

		if (curDomain->IsVariableDefined(curT.ToStdString(), false)) {
			context.AddErrorMessage(curT, "Variable redefination is not allowed in the same code block.");
			return NULL;
		}

		// Test if the coming token is "=", which indicates the variable initialization.
		//
		DataBlock* pInitData = NULL;
		if (allowInit && context.PeekNextToken(0).IsEqual("=")) {
			// TODO: invoken the ParseExpression function to handle it.
		}

		Exp_VarDef* ret = new Exp_VarDef(varType, curT, pInitData);
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
				curDomain->AddDefinedVariable((*it).get()->GetVarName(), (*it).get());
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

bool CompilingContext::ParseSingleExpression(CodeDomain* curDomain)
{
	if (PeekNextToken(0).IsEOF())
		return false;

	if (IsVarDefinePartten(true)) {
		std::vector<Exp_VarDef*>  varDefs;
		if (Exp_VarDef::Parse(*this, curDomain, true, varDefs)) {
			for (int i = 0; i < (int)varDefs.size(); ++i)
				curDomain->AddExpression(varDefs[i]);
		}
		else
			return false;
	}
	else if (IsStructDefinePartten()) {
		Exp_StructDef* structDef = Exp_StructDef::Parse(*this, curDomain);
		if (structDef)
			curDomain->AddExpression(structDef);
		else
			return false;
	}
	else {

		// Try to parse a complex expression
		Exp_ValueEval* valueExp = ParseComplexExpression(curDomain);
		if (!valueExp) {
			Token curT = PeekNextToken(0);
			AddErrorMessage(curT, "Unexpected end of file");
			return false;
		}
		curDomain->AddExpression(valueExp);
	}

	return true;
}

bool CompilingContext::ParseCodeDomain(CodeDomain* curDomain)
{
	if (IsEOF())
		return false;

	bool multiExp = PeekNextToken(0).IsEqual("{");
	if (multiExp) {
		GetNextToken(); // eat the "{"

		CodeDomain* childDomain = new CodeDomain(curDomain);
		curDomain->AddExpression(childDomain);
		if (!ParseCodeDomain(childDomain))
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
		while (ParseSingleExpression(curDomain));
		if (!mErrorMessages.empty())
			return false;
	}

	// The EOF is expected here since all the errors should be caught before and here it should reach the end of file
	return IsEOF();
}

CodeDomain::CodeDomain(CodeDomain* parent)
{
	mpParentDomain = parent;
}

CodeDomain::~CodeDomain()
{

	std::vector<Expression*>::iterator it_exp = mExpressions.begin();
	for (; it_exp != mExpressions.end(); ++it_exp) 
		delete *it_exp;
}

void CodeDomain::AddExpression(Expression* exp)
{
	if (exp)
		mExpressions.push_back(exp);
}

void CodeDomain::AddDefinedType(Exp_StructDef* pStructDef)
{
	if (pStructDef)
		mDefinedStructures[pStructDef->GetStructureName()] = pStructDef;
}

bool CodeDomain::IsTypeDefined(const std::string& typeName)
{
	return mDefinedStructures.find(typeName) != mDefinedStructures.end();
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

Exp_VarDef::Exp_VarDef(VarType type, const Token& var, DataBlock* pData)
{
	mVarType = type;
	mpStructDef = NULL;
	mVarName = var;
	mpDataBlk = pData;
}

Exp_VarDef::~Exp_VarDef()
{
	if (mpDataBlk)
		delete mpDataBlk;
}

void Exp_VarDef::SetStructDef(Exp_StructDef* pStruct)
{
	mpStructDef = pStruct;
}

DataBlock* Exp_VarDef::GetVarDataBlock()
{
	return mpDataBlk;
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
	std::vector<FunctionDomain*>::iterator it_func = mFunctions.begin();
	for (; it_func != mFunctions.end(); ++it_func) 
		delete *it_func;
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

Exp_ValueEval* CompilingContext::ParseSimpleExpression(CodeDomain* curDomain)
{
	Token curT = GetNextToken();
	if (curT.GetType() != Token::kIdentifier && 
		curT.GetType() != Token::kUnaryOp && 
		curT.GetType() != Token::kConstFloat &&
		curT.GetType() != Token::kConstInt) {
		AddErrorMessage(curT, "Invalid token for value expression.");
		return NULL;
	}

	Exp_ValueEval* result = NULL;

	if (curT.GetType() == Token::kIdentifier) {
		// This identifier must be a already defined variable, built-in type or constant
		TypeDesc tpDesc;
		if (IsBuiltInType(curT, &tpDesc)) {
			// Parse and return the built-in type initialize
			if (!ExpectAndEat("(")) return NULL;

			std::auto_ptr<Exp_ValueEval> exp[4];
			int i = 0;
			bool succeed = false;
			Token lastT;
			for (; i < tpDesc.elemCnt; ++i) {
				exp[i].reset(ParseComplexExpression(curDomain));
				if (i == (tpDesc.elemCnt - 1)) {
					lastT = PeekNextToken(0);
					if (lastT.IsEqual(")")) {
						GetNextToken(); // Eat ")"
						succeed = true;
					}
					else
						succeed = false;
				}
				else 
					if (!exp[i].get() && !ExpectAndEat(",")) return NULL;
			}

			if (!succeed) {
				AddErrorMessage(lastT, "Expect \")\".");
				return NULL;
			}
			else {
				Exp_ValueEval* expArray[4] = {exp[0].get(), exp[1].get(), exp[2].get(), exp[3].get()};
				result = new Exp_BuiltInInitializer(expArray, tpDesc.elemCnt, tpDesc.type);
			}
		}
		else if (curDomain->IsVariableDefined(curT.ToStdString(), true)) {
			// Return a value ref expression
			result = new Exp_VariableRef(curT, curDomain->GetVarDefExpByName(curT.ToStdString()));
		}
		else {
			// TODO: if the identifier is a function name, it should return the function call expression
		}
	}
	else if (curT.GetType() == Token::kUnaryOp) {
		// TODO: Generate a unary operator expression
	}
	else {
		// return a constant expression
		if (curT.GetType() != Token::kConstFloat && curT.GetType() != Token::kConstInt) {
			AddErrorMessage(curT, "Unexpected token.");
			return NULL;
		}

		result = new Exp_Constant(curT.GetConstValue(), curT.GetType() == Token::kConstFloat);
	}

	if (!result) {
		AddErrorMessage(curT, "Unexpected token.");
		return NULL;
	}
	// TODO: This is not the end of simple expression, I need to check for the next token to see if it has swizzle.
	// e.g. myVar.xyz, myVar.bgar

	return result;
}

Exp_ValueEval* CompilingContext::ParseComplexExpression(CodeDomain* curDomain)
{
	Exp_ValueEval* simpleExp0 = ParseSimpleExpression(curDomain);
	if (!simpleExp0) {
		// Must have some error message if it failed to parse a simple expression
		assert(!mErrorMessages.empty()); 
		return NULL;
	}

	Token curT = GetNextToken();
	if (curT.IsEqual(";")) {
		return simpleExp0;
	}
	if (curT.GetType() != Token::kBinaryOp) {
		AddErrorMessage(curT, "Expect a binary operator.");
		return NULL;
	}
	int op0_level = curT.GetBinaryOpLevel();
	std::string op0_str = curT.ToStdString();

	Exp_ValueEval* simpleExp1 = ParseSimpleExpression(curDomain);
	if (!simpleExp1) {
		// Must have some error message if it failed to parse a simple expression
		assert(!mErrorMessages.empty()); 
		return NULL;
	}

	// Get the next token to decide if the next binary operator is in high level of priority
	Token nextT = GetNextToken();
	if (nextT.IsEqual(";")) {
		// we are done for this complex value expression, return it.
		Exp_BinaryOp* pBinaryOp = new Exp_BinaryOp(op0_str, simpleExp0, simpleExp1);
		return pBinaryOp;
	}
	else if (nextT.GetType() == Token::kBinaryOp) {
		int op1_level = nextT.GetBinaryOpLevel();
		std::string op1_str = nextT.ToStdString();

		if (op1_level > op0_level) {
			Exp_ValueEval* simpleExp2 = ParseComplexExpression(curDomain);
			Exp_BinaryOp* pBinaryOp1 = new Exp_BinaryOp(op1_str, simpleExp1, simpleExp2);
			Exp_BinaryOp* pBinaryOp0 = new Exp_BinaryOp(op0_str, simpleExp0, pBinaryOp1);
			return pBinaryOp0;
		}
		else {
			Exp_ValueEval* simpleExp2 = ParseComplexExpression(curDomain);
			Exp_BinaryOp* pBinaryOp0 = new Exp_BinaryOp(op1_str, simpleExp0, simpleExp1);
			Exp_BinaryOp* pBinaryOp1 = new Exp_BinaryOp(op0_str, pBinaryOp0, simpleExp2);
			return pBinaryOp1;
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

VarType Exp_Constant::GetValueType()
{
	return mIsFromFloat ? VarType::kFloat : VarType::kInt;
}

Exp_VariableRef::Exp_VariableRef(Token t, Exp_VarDef* pDef)
{
	mVariable = t;
	mpDef = pDef;
}

Exp_VariableRef::~Exp_VariableRef()
{

}

VarType Exp_VariableRef::GetValueType()
{
	return mpDef->GetVarType();
}

Exp_StructDef* Exp_VariableRef::GetStructDef()
{
	if (GetValueType() == VarType::kStructure) 
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

VarType Exp_BuiltInInitializer::GetValueType()
{
	return mType;
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

VarType Exp_UnaryOp::GetValueType()
{
	// TODO: will unary operation change the value type?
	return mpExpr->GetValueType();
}


VarType Exp_BinaryOp::GetValueType()
{
	return mpLeftExp->GetValueType();
}

} // namespace SC