
void theloop(unsigned int count) {
	unsigned int temp = 0;
	for (unsigned int i = 0, e = count; i < e; ++i) {
		temp++;
	}
}

int main (int argc, char** argv)
{
	float a = 1.0 * 2;
	double b = 4.5 / (23 << 1) * a;
	theloop(20);
	theloop(1U << 20);
	return 0;
}

