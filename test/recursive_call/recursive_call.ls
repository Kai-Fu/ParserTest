// This is comments..

int factorial(int arg) 
{
	int ret;
	if (arg < 2)
		return 1;
	else
		return arg * factorial(arg - 1);
}

