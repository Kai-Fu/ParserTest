// The following line is to tell the parser that the following global variable is used as shader parameters.
%instance
{
	int myIntParam1 = 0;
	int3 myInt3Param2 = int3(1, 2, 3);

	float myFloatParam1 = 0.1234;
	float4 myFloatParam2 = float4(0.1, 0.2, 0.3, 0.4);
}

// These variables are shader based, which means their modification will be reflected by every
// shader instances that use this shader code.
int2 myPrivatedParam1 = int2(4,5);
float3 myPrivateParam2 = float3(0.6, 0.7, 0.8);

struct TestStruct1
{
	float3 a;
	int2 b;
};

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

