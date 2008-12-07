#include  <stdio.h>

void function1(char * p);
void function2(char * p);

void function1(char * p)
{
	///  double d=0;
	///  for( unsigned int j=0 ; j<15 ; j++ )
	///  {
	///    d++;
	///  }
	double d=0;
	d++;
	function2(p);
}

void function2(char * p)
{
}

int main(int argc, char *argv[])
{
	// Dynamic optimizer test
	double d=0;
	int q = 7;
	printf("Running main\n");

	for( unsigned int i=0 ; i<30 ; i++ )
	{
		function1("function1");
	}

	// Multiloops
	for (int i = 0; i < 30; i++) {
		q++;
	}

	for (int i = 0; i < 30; i++) {
		q = q << 1;
		q = q >> 1;
	}

	for (int i = 0; i < 30; i++) {
		q *= 2;
	}

	printf("Done\n");
}
