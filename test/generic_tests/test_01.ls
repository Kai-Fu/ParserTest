// This is comments..

void CompareTwoInt(int a, int b);

float DotProduct_0(float3 l, float3 r) 
{
	float3 tmp = l * r;
	return tmp.x + tmp.y + tmp.z;
}

float DotProduct_1(float3 l, float3 r) 
{
	return l.x*r.x + l.y*r.y + l.z*r.z;
}

int run_test()
{
	float res0 = DotProduct_0(float3(0.2, 0.3, 0.4), float3(0.4, 0.13, 0.54));
	float res1 = DotProduct_1(float3(0.2, 0.3, 0.4), float3(0.4, 0.13, 0.54));
	CompareTwoInt(res0*1000, res1*1000);
	return 0;
}