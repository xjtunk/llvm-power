#include  "gatinglib.h"

// Useful functions for converting a bitmask into individual functional unit indices.
#define LOG2(x) (ffs(x)-1)

// Useful for comparing doubles. Instead of double d==double a, use abs(d-a)<EPSILON.
// Because we are trying to model unlimited precision using limited number of bits.
#define EPSILON 1e-5

bool FunctionalUnitManager::functionalUnitAvailable(const unsigned long &unitNumber, const unsigned long long &now)
{
  synchronize(unitNumber, now);
  if(functionalUnits[unitNumber].status==FUS_ON)
    return true;
  else
    return false;
}

void FunctionalUnitManager::gateWithMask(const unsigned long long &mask, const unsigned long long &now)
{
}

unsigned long long FunctionalUnitManager::turnOff(const unsigned long &unitNumber, const unsigned long long &now)
{
  synchronize(unitNumber, now);
  if(functionalUnits[unitNumber].status==FUS_OFF)
    return 0;
  else if(functionalUnits[unitNumber].status==FUS_ON)
  {
  	
  	functionalUnits[unitNumber].transitionTime = now;
    functionalUnits[unitNumber].nextClock=now+functionalUnits[unitNumber].offLatency;
    functionalUnits[unitNumber].lastPeakPower = functionalUnits[unitNumber].onPower;
    functionalUnits[unitNumber].totalPower += functionalUnits[unitNumber].onPower * (now - functionalUnits[unitNumber].onTime);
    assert((now - functionalUnits[unitNumber].onTime) >= 0);
  }
  else if(functionalUnits[unitNumber].status==FUS_ON_TRANSITION)
  {
  	
  	//if transition occured from another transition, figure out that point
  	unsigned long long lastTransition = (unsigned long long)(((double)functionalUnits[unitNumber].onLatency/functionalUnits[unitNumber].onPower)*functionalUnits[unitNumber].lastPeakPower);
  	
  	//Using parametrized line, figure out what instantateous power is
  	unsigned long long chargedLatency = now - functionalUnits[unitNumber].transitionTime;
  	double currentPower = (functionalUnits[unitNumber].onPower/(double)functionalUnits[unitNumber].onLatency)*(double)(chargedLatency + lastTransition);
  	
  	
  	//Only add power that has been consumed
  	double powerConsumed = .5 * currentPower * (double)chargedLatency;
  	double powerNotConsumed = .5 * functionalUnits[unitNumber].lastPeakPower * lastTransition;
  	functionalUnits[unitNumber].totalPower +=  (powerConsumed - powerNotConsumed);
  	assert((powerConsumed - powerNotConsumed) >= 0);

    // FIXME: Currently suffers all the latency for turning off, though 
    // FIXED!!!
    // it is not fully turned on.
    
    //Find the intersection of the off transition curve and on transition curve (line because we're not doing exponentials)
    double transitionPoint = (functionalUnits[unitNumber].onPower - currentPower)*((double)functionalUnits[unitNumber].offLatency/functionalUnits[unitNumber].onPower);
    
    functionalUnits[unitNumber].nextClock=now+(functionalUnits[unitNumber].offLatency - (unsigned long long)transitionPoint);
    functionalUnits[unitNumber].transitionTime = now;
    functionalUnits[unitNumber].lastPeakPower = currentPower;
    assert(functionalUnits[unitNumber].nextClock > now);
    
  }
  functionalUnits[unitNumber].status=FUS_OFF_TRANSITION;
  return functionalUnits[unitNumber].nextClock;
}

unsigned long long FunctionalUnitManager::turnOn(const unsigned long &unitNumber, const unsigned long long &now)
{
  synchronize(unitNumber, now);
  if(functionalUnits[unitNumber].status==FUS_ON)
    return 0;
  else if(functionalUnits[unitNumber].status==FUS_OFF)
  {
  	functionalUnits[unitNumber].transitionTime = now;
    functionalUnits[unitNumber].nextClock=now+functionalUnits[unitNumber].onLatency;
    functionalUnits[unitNumber].lastPeakPower = 0; 
    
    //do nothing with regards to power accum or timing
  }
  else if(functionalUnits[unitNumber].status==FUS_OFF_TRANSITION)
  {
  	unsigned long long prevOnTime, chargedLatency;
  	double powerTotal, currentPower, powerNotConsumed, transitionPoint;
  	
  	//Take total transition triangle power (or time since last transition)
  	prevOnTime = functionalUnits[unitNumber].nextClock - functionalUnits[unitNumber].transitionTime;
	  powerTotal =  .5 * functionalUnits[unitNumber].lastPeakPower * (double)prevOnTime; //1/2 base * height (fixed.. if there are two mid transitions in a row or something
  
  	//subtract smaller triangle
  	//Using parametrized line, figure out what instantateous power is
  	chargedLatency = now - functionalUnits[unitNumber].transitionTime;
  	currentPower = functionalUnits[unitNumber].onPower - (functionalUnits[unitNumber].onPower/((double)prevOnTime))*(double)((functionalUnits[unitNumber].offLatency - prevOnTime) + chargedLatency); //instantaneous Power
  	
  	
  	//Only add power that has been consumed
  	powerNotConsumed = .5 * currentPower * (double)(prevOnTime - chargedLatency);
  
  	//Subtract larger triangle from smaller one
  	functionalUnits[unitNumber].totalPower += (powerTotal - powerNotConsumed);
  	assert((powerTotal - powerNotConsumed) >= 0);
  	//calculate transition point, and new latency
  	transitionPoint = currentPower*((double)functionalUnits[unitNumber].onLatency/functionalUnits[unitNumber].onPower);
  	
    // FIXME: Currently suffers all the latency for turning off, though
    // FIXED!!!
    // it is not fully turned on.
    functionalUnits[unitNumber].nextClock=now+(functionalUnits[unitNumber].onLatency - (unsigned long long)transitionPoint);
    
    functionalUnits[unitNumber].transitionTime = now;
    functionalUnits[unitNumber].lastPeakPower = currentPower;
    assert(functionalUnits[unitNumber].nextClock > now);
  }
  functionalUnits[unitNumber].status=FUS_ON_TRANSITION;
  return functionalUnits[unitNumber].nextClock;
}

double FunctionalUnitManager::getTotalPower()
{
	double totalPower = 0;
	for(int i = 0; i < functionalUnits.size(); i++)
		totalPower += functionalUnits[i].getTotalPower();
	
	return totalPower;
}

void FunctionalUnitManager::synchronize(const unsigned long &unitNumber, const unsigned long long &now)
{
  assert(unitNumber<functionalUnits.size());
  if(now==globalClock)
    return;
  if(functionalUnits[unitNumber].status==FUS_ON_TRANSITION &&
      functionalUnits[unitNumber].nextClock<=now)
 	{
    functionalUnits[unitNumber].status=FUS_ON; 
  	functionalUnits[unitNumber].onTime = now;

  	
  	//first check to see if an entire transition time occurred
  	if(now - functionalUnits[unitNumber].transitionTime < functionalUnits[unitNumber].onLatency)
  	{
  		//partial triangle
  		double powerTotal = .5 * functionalUnits[unitNumber].onPower * functionalUnits[unitNumber].onLatency;
  		
  		//figure out transition point (this stuff can obviously be optimized.. but readability more important than full opt)
  		double transitionPower = functionalUnits[unitNumber].onPower/(double)functionalUnits[unitNumber].onLatency*((double)functionalUnits[unitNumber].onLatency - (double)functionalUnits[unitNumber].transitionTime);
  		
  		double powerNotConsumed = .5 * transitionPower * ((double)functionalUnits[unitNumber].onLatency - (double)functionalUnits[unitNumber].transitionTime);
  		
  		functionalUnits[unitNumber].totalPower += (powerTotal - powerNotConsumed);
  	  assert((powerTotal - powerNotConsumed) >= 0);
  	
  	}
  	else
  	{
  		
  		//full triangle
  		functionalUnits[unitNumber].totalPower += .5 * functionalUnits[unitNumber].onPower * functionalUnits[unitNumber].onLatency;

  	
  	}
  	
  	functionalUnits[unitNumber].lastPeakPower = functionalUnits[unitNumber].onPower;
  	
  }
  if(functionalUnits[unitNumber].status==FUS_OFF_TRANSITION &&
      functionalUnits[unitNumber].nextClock<=now)
  {
    functionalUnits[unitNumber].status=FUS_OFF; 
  	
  	//first check to see if an entire transition time occurred
  	if(now - functionalUnits[unitNumber].transitionTime < functionalUnits[unitNumber].offLatency)
  	{
  		//Just add smaller triangle
  		
  		double transitionPower = functionalUnits[unitNumber].onPower - (functionalUnits[unitNumber].onPower/(double)functionalUnits[unitNumber].offLatency)*((double)functionalUnits[unitNumber].offLatency - (double)functionalUnits[unitNumber].transitionTime);
  		
  		functionalUnits[unitNumber].totalPower += .5 * transitionPower * (double)functionalUnits[unitNumber].transitionTime;
  		
  	
  	}
  	else
  	{
   		//full triangle
  		functionalUnits[unitNumber].totalPower += .5 * functionalUnits[unitNumber].onPower * functionalUnits[unitNumber].onLatency; 	
   	
  	}		
		
		functionalUnits[unitNumber].lastPeakPower = 0;

	}
}

void FunctionalUnitManager::synchronize(const unsigned long long &now)
{
  if(now==globalClock)
    return;
  for( unsigned int i=0 ; i<functionalUnits.size() ; i++ )
  {
/*    if(functionalUnits[i].status==FUS_ON_TRANSITION &&
       functionalUnits[i].nextClock<=now)
     functionalUnits[i].status=FUS_ON; 
    if(functionalUnits[i].status==FUS_OFF_TRANSITION &&
       functionalUnits[i].nextClock<=now)
     functionalUnits[i].status=FUS_ON; 
  */
  	synchronize(i, now);
  
  }
  globalClock=now;
}

int FunctionalUnitManager::readFunctionalUnitFile(const char * filename)
{
  unsigned int size=0;
  stringbuf sb;
  istream infile;
  dynarray<char*> tokens;

  infile.open(filename);
  if(!infile)
    return -1;

  while(infile.size()!=infile.where())
  {
    sb.reset();
    infile.readline(sb);
    // tokenize it using spaces. See how many tokens we have.
    tokens.tokenize(sb, " ");
    // ignore comment tokens and !=4 lines
///    printf("token string: %s index: %d # of tokens: %d position: %lld\n", (char*)sb, size, tokens.size(), infile.where());
    if(tokens.size()!=4 || tokens[0][0]=='#')
      continue;
    size++;
    functionalUnits.resize(size);
    functionalUnits[size-1].name = tokens[0];
    functionalUnits[size-1].onPower = atof(tokens[1]);
    functionalUnits[size-1].offLatency = atoll(tokens[2]);
    functionalUnits[size-1].onLatency = atoll(tokens[3]);
  }
  infile.close();

  return 0;
}

void FunctionalUnitManager::dumpFunctionalUnits()
{
  printf("Printing functional units\n");
  printf("Name\tonPower\toffLatency\tonLatency\n");
  for( unsigned int i=0 ; i<functionalUnits.size() ; i++ )
  {
    printf("%s\t%lf\t%lld\t%lld\n",
    (char*)functionalUnits[i].name,
    functionalUnits[i].onPower,
    functionalUnits[i].offLatency,
    functionalUnits[i].onLatency);
  }
  printf("Done printing functional units\n");
}

#undef LOG2
#undef EPSILON
