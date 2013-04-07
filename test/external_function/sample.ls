// This is comments..

float SimpleCallee(float& arg);

struct ArrayCntr {
	float abc;
	float my_array[13];
};

/*float SimpleCallee(float& arg) 
{
	arg = arg + 123.5f;
	return arg;
}*/

void HandleArray(ArrayCntr& ref)
{
	float3 my_array[13];

	my_array[2] = float3(2,3,4);
	float mod_arg = 321;
	SimpleCallee(mod_arg);

	float abc[3];
	abc[2] = -124;
	SimpleCallee(abc[2]);
	if (abc[2] > 0) {
		ref.my_array[0] = abc[2];
		ref.my_array[2] = 100;
		}
	else {
		ref.my_array[0] = 100;
		ref.my_array[3] = 100;
	}



	my_array[2].z = mod_arg;

	ref.my_array[7] = my_array[2].z;
	return;
}
