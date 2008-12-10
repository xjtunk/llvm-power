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
	d *= 3.4;
	function2(p);
}

void function2(char * p)
{
}

int main(int argc, char *argv[])
{
	// Dynamic optimizer test
	double d=1.2, e=7.7;
	int q = 7;
	int r = 7;
	int s = 7;
	d = d * e;
	printf("Running main\n");

	for( unsigned int i=0 ; i<30 ; i++ )
	{
		function1("function1");
	}

	// Multiloops
	for (int i = 0; i < 30; i++) {
		q++;
		for (int j = 0; j < 20; j++) {
			r++;
		}
	}

	for (int i = 0; i < 30; i++) {
		for (int j = 0; j < 40; j++) {
			r++;
		}
		q = q << 1;
		q = q >> 1;
	}

	for (int i = 0; i < 30; i++) {
		for (int j = 0; j < 30; j++) {
			q *= 2;
			for (int k = 0; k < 30; k++) {
				s++;
				d *= e * 3.4;
			}
		}
	}

	printf("Done %f %f %d\n", d, e, q);
}
