// This is comments..

   
/* this is 
different comments.
*/

// 1234  abcdf.dfe  123.456f

struct TestStruct1
{
	float a;
	/* This is commnent  */
	bool cond;
	int2 ib;
	float b;
	
};
/*
struct NestedStruct
{
	float a;
	bool my_b;
	float3 my_array[7];
	TestStruct1 s;
};

float DistanceSqr(float a, float b)
{
	NestedStruct varStruct;
	varStruct.a = 3.0;
	varStruct.my_b = true;
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
	mod.cond = (mod.a < mod.b && 123 > 123.8);
	float var = 123.891f + 321.0f;
	mod.ib = int2(123, 234) & int2(567, 876);
	return var;
}


float HandleVector(float3& ref)
{
	float3 a = float3(2.0, 3.0, 4.0);
	float4 b = float4(a, 123);
	ref = a.zyx;
	ref.y = 145.6;
	ref.z = b.w;
	return 3.14f;
}*/

struct ArrayCntr {
	float abc;
	float my_array[13];
};

float HandleArray(ArrayCntr& ref)
{
	/*float3 my_array[13];

	my_array[2] = float3(2,3,4);
	my_array[2].z = 1;*/

	ref.my_array[7] = 0.123;
	return 123.6;
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