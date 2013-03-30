// This is comments..

   
/* this is 
different comments.
*/

// 1234  abcdf.dfe  123.456f

struct TestStruct1
{
	float a;
	/* This is commnent  */
	float b;
};

struct NestedStruct
{
	float a;
	TestStruct1 s;
};

float DistanceSqr(float a, float b)
{
	NestedStruct varStruct;
	varStruct.a = 3.0;
	varStruct.s.b = 4.0;
	return a*a + b*b + varStruct.a * varStruct.s.b;
}

int TwoIntegerBinary(int a, int b)
{
	return a * b;
}

float FloatAndIntBinary(float a, int b)
{
	return a + b;
}

float RetSimpleValue(TestStruct1& mod)
{
	mod.a = 234.888f;
	mod.b = 678.9;
	float var = 123.891f + 321.0f;
	return var;
}


float HandleVector(float3& ref)
{
	float3 a = float3(2.0,3,4.0);
	ref = a;
	return 3.14f;
}


/*
float3 simpleFunc(float2& a, float b)
{

TestStruct1 myStructVar0;

int myIntParam1 = 0;
int myIntParam2;
int3  myInt3;
float myTestVar0;
bool bTest = true;
bTest = false;
	int c = b + a;
	myTestVar0 = myIntParam1 + myTestVar0 * myIntParam2;
	int3 myInt3Param2 = int3(1, 2, 3) + myInt3 * (myStructVar0.a + float3(3,4,5));
	float4 myFloat4 = myInt3.yxz.rbbg;
	myFloat4 = -myInt3Param2.zyxx;
	return ReturnFloat2(c, 123.0f).xxyy;
}
*/


/*
int3 myInt3Param2 = int3(1, 2, 3);

float myFloatParam1 = 0.1234;
float4 myFloatParam2 = float4(0.1, 0.2, 0.3, 0.4);

myFloatParam2.yxwz;


// These variables are shader based, which means their modification will be reflected by every
// shader instances that use this shader code.
int2 myPrivatedParam1 = int2(4,5);
float3 myPrivateParam2 = float3(0.6, 0.7, 0.8);


struct TestStruct2
{
	float4 a;
	int4 b;
};

TestStruct1 myStructParam1 = {float3(1,2,3), int2(4,5)}

// Function declarations
void ComputeLighting(TestStruct1& input, TestStruct2& output)
{
	output.a = intput.b.xyxy + float4(2,3,4,6) * float4(input.a.xyz, 1);
	output.b = int4(5,6,7,8);
}

*/

//TestStruct1 myStruct001;

// Bad struct defination
//BadTestStruct myStruct002;