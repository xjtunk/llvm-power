#include  "gatinglib.h"

int main(int argc, char *argv[])
{
  unsigned long long now;
  unsigned long long latency;
  FunctionalUnitManager fum;
  fum.readFunctionalUnitFile("fu.txt");
  fum.dumpFunctionalUnits();

  // let's turn off the adder
  now = 100;
#define TEST_AND_PRINT(expr) {latency=expr; printf("%s returned: %lld\n", #expr, latency);printf("Total Power: %f\n", fum.getTotalPower());}
  TEST_AND_PRINT(fum.turnOff(0, 100));
  TEST_AND_PRINT(fum.turnOn(0, 118));
  TEST_AND_PRINT(fum.turnOff(0, 123));
  TEST_AND_PRINT(fum.turnOn(0, 125));
  TEST_AND_PRINT(fum.turnOff(0, 128));
  TEST_AND_PRINT(fum.turnOff(0, 160));
  TEST_AND_PRINT(fum.turnOn(0, 190));
#undef TEST_AND_PRINT
  return 0;
}
