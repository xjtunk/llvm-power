
#include <cstdio>

void funcA() {
	printf("In function: %s\n", __FUNCTION__);
	int a = 45;
	int b = 20;
	while (b > 0)
		b--;
	int d = a - b;
}

void funcB() {
	printf("In function: %s\n", __FUNCTION__);
	int a = 45;
	int b = 20;
	while (b > 0)
		b--;
	int d = a - b;
}

void funcC() {
	printf("In function: %s\n", __FUNCTION__);
	int a = 45;
	int b = 20;
	while (b > 0)
		b--;
	int d = a - b;
}

void funcD() {
	printf("In function: %s\n", __FUNCTION__);
	int a = 45;
	int b = 20;
	while (b > 0)
		b--;
	int d = a - b;
}

void funcE() {
	printf("In function: %s\n", __FUNCTION__);
	int a = 45;
	int b = 20;
	while (b > 0)
		b--;
	int d = a - b;
}

int main ()
{
	int a = 77;
	while (a-- > 0) {
		switch (a%5) {
			case 0: funcA(); break;
			case 1: funcB(); break;
			case 2: funcC(); break;
			case 3: funcD(); break;
			case 4: funcE(); break;
		}
	}
	return 0;
}

