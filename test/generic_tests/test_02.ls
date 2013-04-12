// This is comments..

void CompareTwoInt(int a, int b);

float TestFunc_0(float l, float r) 
{
	return pow(l, r);
}

extern MyExtType;

float ExternFunc(MyExtType arg)
{
	MyExtType tmp = arg;
	bool tmpB = true;
	bool tmpA = !tmpB;
	return 123.45;
}

float TestFunc_1(float l) 
{
	return sqrt(l);
}

int run_test()
{
	float res0 = TestFunc_0(2, 0.5);
	float res1 = TestFunc_1(2);
	CompareTwoInt(res0*1000, res1*1000);
	return 0;
}