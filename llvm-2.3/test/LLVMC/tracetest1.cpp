#include  <stdio.h>

int main(int argc, char *argv[])
{
  double d=0;
  for( int i=0 ; i<100 ; i++ )
  {
    for( int j=0 ; j<100 ; j++ )
    {
      for( int k=0 ; k<100 ; k++ )
      {
        if(i>1)
        {
          d++;
        }
        else
        {
          d--;
        }
      }
    }
    for( int j=0 ; j<2 ; j++ )
    {
      if(j>=0)
      {
        d+=2;
      }
    }
    
  }
  return 0;
}
