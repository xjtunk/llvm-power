
int main ()
{
	long long int c = 0;
	while(1) {
		c++;
		if (c > 1LL<<30) {
			break;
		}
	}
	return 0;
}

