// This is comments..

struct TestStructure
{
	float4 var0;  // float3
	int2 var1; // int2
};

int PFN_RW_Structure(TestStructure& arg, TestStructure% arg1) 
{
	arg.var0 = float4(0.1, 0.2, 0.3, 0.4);
	arg.var1 = int2(5,6);
	
	arg.var0 = arg.var0 + arg1.var0;
	arg.var1 = arg.var1 + arg1.var1;
	return 123;
}

