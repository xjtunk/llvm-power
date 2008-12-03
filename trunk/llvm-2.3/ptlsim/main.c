#include  "ptlcalls.h"

#define GATE_INSTR(x) asm(".byte 0x0f; .byte 0x38; .long "#x);

void break_fn(W64 mask);

int main(int argc, char *argv[])
{
///  ptlcall_marker(7);
  break_fn(0);
  return 0;
}

void break_fn(W64 mask)
{
///  asm(".byte 0x0f; .byte 0x3f; .long 0x77777777;");
  GATE_INSTR(43);
///  asm("movl %ecx, %eax");
}
