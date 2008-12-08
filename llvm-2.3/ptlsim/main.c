#include  "ptlcalls.h"

#define GATE_INSTR(x,y) asm(".byte 0x0f; .byte 0x38; .long "#x "; .long "#y);

void break_fn(W64 mask);

int main(int argc, char *argv[])
{	
	int n, k;
	for(k= 0; k < 1000; k++)
		n = n + 1;
	printf("N = %d", n);
	asm(".byte 0x0f; .byte 0x39");
///  ptlcall_marker(7);
	printf("Start\n");
 break_fn(0);
 
 	
  printf("Finished\n");
  return 0;
}

void break_fn(W64 mask)
{
	int i;
	int j = 0;
///  asm(".byte 0x0f; .byte 0x3f; .long 0x77777777;");
  GATE_INSTR(0,0);
///  asm("movl %ecx, %eax");
	for(i = 0; i < 1000; i++)
		j = j + 1;
	GATE_INSTR(0xFF,0xFF);
	for(i = 0; i < 1000; i++)
		j = j+1;
	GATE_INSTR(0xF0, 0xF0);
	printf("J = %d", j);
}
