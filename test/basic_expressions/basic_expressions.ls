// This is comments..

int TestBooleanValues() 
{
	bool isTrue0 = 3.1 > 2.0;
	bool isTrue1 = 4 > 2;
	bool isTrue2 = (5+7.0) >= 11.2f;
	
	int ret = 0;
	if (isTrue0) ret = ret + 1;
	if (isTrue1) ret = ret + 1;
	if (isTrue2) ret = ret + 1;
	
	return ret;
}

struct _Float3
{
	float v[3];
};

float DotProduct(_Float3& l, _Float3& r)
{
	float3 ll = float3(l.v[0], l.v[1], l.v[2]);
	float3 rr = float3(r.v[0], r.v[1], r.v[2]);
	float3 tmp = ll * rr;
	return tmp.x + tmp.y + tmp.z;
}

float TestSwizzle(_Float3& ref)
{
	float3 tmp = float3(ref.v[0], ref.v[1], ref.v[2]);
	float3 tmp1 = tmp.zyx;
	float4 tmp2 = float4(tmp1, 123.0f);
	ref.v[0] = tmp2.y;
	ref.v[1] = tmp2.w;
	ref.v[2] = tmp2.z;
	return ref.v[0];
}