#include  <stdio.h>

void function1(char * p);
void function2(char * p);

void function1(char * p)
{
  function2(p);
}

void function2(char * p)
{
}

int main(int argc, char *argv[])
{
  // Dynamic optimizer test
  double d=0;
  printf("Running main\n");

  for( unsigned int i=0 ; i<30 ; i++ )
  {
    function1("function1");
  }
  printf("Done\n");
}
