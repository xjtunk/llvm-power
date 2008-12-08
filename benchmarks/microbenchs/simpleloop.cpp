#include <cstdio>
#include <cstdlib>
void theloop(int count) {
	unsigned int temp = 0;
	for (int i = 0, e = count; i < e; ++i) {
		temp++;
	}
	printf("total: %u\n", temp);
}

int main (int argc, char** argv)
{
	theloop(atoi(argv[1]));
	theloop(atoi(argv[2]));
	return 0;
}

