#include  <stdio.h>

void function1(char * p);
void function2(char * p);

void function1(char * p)
{
  printf("inside function: %s\n", __FUNCTION__);
  function2(p);
}

void function2(char * p)
{
}

int main(int argc, char *argv[])
{
  // Dynamic optimizer test
  printf("Running main\n");

  for( unsigned int i=0 ; i<100 ; i++ )
  {
    function1("hello");
  }
  printf("Done\n");
}
