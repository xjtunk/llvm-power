#include  "ptlcalls.h"

#define GATE_INSTR(x,y) asm(".byte 0x0f; .byte 0x38; .long "#x "; .long "#y);

void break_fn(W64 mask);

int main(int argc, char *argv[])
{
///  ptlcall_marker(7);
	printf("Start\n");
  break_fn(0);
  printf("Finished\n");
  return 0;
}

void break_fn(W64 mask)
{
///  asm(".byte 0x0f; .byte 0x3f; .long 0x77777777;");
  GATE_INSTR(0,0);
///  asm("movl %ecx, %eax");
	GATE_INSTR(0xFF,0xFF);
}
