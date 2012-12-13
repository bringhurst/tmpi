# include <stdio.h>
# define type_size(type) printf("sizeof(" #type ")=%d\n", sizeof(type))
int main()
{
	type_size(int);
	type_size(long);
	type_size(long long);
	type_size(char);
	type_size(short);
	type_size(float);
	type_size(double);
	type_size(long double);

	return 0;
}
