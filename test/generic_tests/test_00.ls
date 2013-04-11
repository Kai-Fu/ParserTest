// This is comments..

void CompareTwoInt(int a, int b);

int factorial_0(int arg) 
{
	int ret = 1;
	for (int i = 1; i <= arg; i = i+1)
		ret = ret * i;
	return ret;
}

int factorial_1(int arg) 
{
	int ret;
	if (arg < 2)
		return 1;
	else
		return arg * factorial_1(arg - 1);
}

int run_test()
{
	int res0 = factorial_0(10);
	int res1 = factorial_1(10);
	CompareTwoInt(res0, res1);
	return 0;
}