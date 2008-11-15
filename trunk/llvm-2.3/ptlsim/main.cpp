#include  "gatinglib.h"
#define OFF		0
#define ADD		1
#define MULT	2
#define DIV		4

int main(int argc, char *argv[])
{
  unsigned long long now;
  unsigned long long latency;
  FunctionalUnitManager fum("fu.txt");

  fum.dumpFunctionalUnits();

  // let's turn off the adder
  now = 100;
  
  
  #define ISSUE(mask, time) { fum.processAtIssue(mask, time); cout<<"ISSUE @ "<<time<<" Total Power: "<<fum.getTotalPower(time)<<endl; fum.dumpStats(time);} 
  #define COMMIT(mask, time) { fum.processAtCommit(mask, time); cout<<"COMMIT @ "<<time<<" Total Power: "<<fum.getTotalPower(time)<<endl; fum.dumpStats(time);} 
  
	ISSUE(OFF, 100);
	ISSUE(ADD, 110);
	COMMIT(OFF, 130);

	ISSUE(ADD , 200);

	
	
  return 0;
}
