#include  <stdio.h>

int main(int argc, char *argv[])
{
  double d=0;
  for( int i=0 ; i<100 ; i++ )
  {
    if(i>5)
    {
      d++;
    }
    else
    {
      d--;
    }
  }
  return 0;
}
