#include  "gatinglib.h"
#include <cassert>

// Useful functions for converting a bitmask into individual functional unit indices.
#define LOG2(x) (ffs(x)-1)

// Useful for comparing doubles. Instead of double d==double a, use abs(d-a)<EPSILON.
// Because we are trying to model unlimited precision using limited number of bits.
#define EPSILON 1e-5


void FunctionalUnitManager::processAtIssue(const unsigned int &mask, const tick_t &now)
{
	int i;
	unsigned int _mask = mask; 
	for(i = 0; i < functionalUnits.size(); i++)
	{
		if(_mask & 0x1)
		{
			functionalUnits[i]->turnOn(now);	
			functionalUnits[i]->updateInPipeline(); //indicate that a turnon has been dispatched but not commited
		}

		_mask >>= 1;
	}

}

void FunctionalUnitManager::processAtCommit(const unsigned int &mask, const tick_t &now)
{
	int i;
	unsigned int _mask = mask;
	for(i = 0; i < functionalUnits.size(); i++)
	{
		if(_mask & 0x1)
		{
			functionalUnits[i]->updateNotInPipeline();
		}
		else
		{
			if(!functionalUnits[i]->getUpdateStatus())//only turn off an FU if a turn on hasnt been dispatched and not commited
			{
				functionalUnits[i]->turnOff(now);
			}
		
		}
		_mask >>= 1;
	}
}

void FunctionalUnitManager::processAtFlush(const unsigned int &mask, const tick_t &now)
{



}


bool FunctionalUnitManager::functionalUnitAvailable(const unsigned long &unitNumber, const tick_t &now)
{
  functionalUnits[unitNumber]->synchronize(now);
  if(functionalUnits[unitNumber]->checkAvailable())
    return true;
  else
    return false;
}



tick_t FunctionalUnit::turnOff(const tick_t &now)
{
  synchronize(now);
  
  if(status==FUS_OFF)
  {
     #if DEBUG
    	cout<<now<<": "<<getName()<<" OFF -> OFF"<<endl;
    #endif
  	return 0;  
  } 
  else if(status == FUS_ON)
  {
  	
  	transitionTime = now;
    nextClock=now + offLatency;
    lastPeakPower = onPower;
    totalPower += onPower * (double)(now - onTime);
    
    totalOnTime += (now - onTime);
    
    assert((now - onTime) >= 0);
    
    #if DEBUG
    	cout<<now<<": "<<getName()<<" ON -> OFF_TRANSITION"<<endl;
    #endif

  }
  else if(status==FUS_ON_TRANSITION)
  {
  	
  	//if transition occured from another transition, figure out that point
  	tick_t lastTransition = (tick_t)( ((double)onLatency /onPower) * lastPeakPower);
  	
  	//Using parametrized line, figure out what instantateous power is
  	tick_t chargedLatency = now - transitionTime;
  	
  	//update stat
  	totalOnTransitionTime += chargedLatency;
  	
  	//Calculate energy right now
  	double currentPower = (onPower/(double)onLatency)*(double)(chargedLatency + lastTransition);
  	
  	
  	//Only add power that has been consumed
  	power_t powerConsumed = .5 * currentPower * (double)chargedLatency;
  	power_t powerNotConsumed = .5 * lastPeakPower * lastTransition;
  	
  	totalPower +=  (powerConsumed - powerNotConsumed);
  	
  	assert((powerConsumed - powerNotConsumed) >= 0);
    
    //Find the intersection of the off transition curve and on transition curve 
    double transitionPoint = (onPower - currentPower) * ((double)offLatency/onPower);
    
    nextClock = now + (offLatency - (tick_t)transitionPoint);
    transitionTime = now;
    lastPeakPower = currentPower;
    
    assert(nextClock > now);
    
    #if DEBUG
    	cout<<now<<": "<<getName()<<" ON_TRANSITION -> OFF_TRANSITION"<<endl;
    #endif
    
  }
  
  status = FUS_OFF_TRANSITION;
  return nextClock;
}

tick_t FunctionalUnit::turnOn(const tick_t &now)
{
  synchronize(now);
  
  if(status == FUS_ON)
  {
    #if DEBUG
    	cout<<now<<": "<<getName()<<" ON -> ON"<<endl;
    #endif 
  	return 0;
  } 
  else if(status == FUS_OFF)
  {
  	transitionTime = now;
    nextClock=now + onLatency;
    lastPeakPower = 0; 
    
    //update stats
    totalOffTime += (now - transitionTime);
    #if DEBUG
    	cout<<now<<": "<<getName()<<" OFF -> ON_TRANSITION"<<endl;
    #endif 
    //do nothing with regards to power accum or timing
  }
  else if(status == FUS_OFF_TRANSITION)
  {
  	unsigned long long prevOnTime, chargedLatency;
  	double powerTotal, currentPower, powerNotConsumed, transitionPoint;
  	
  	//Take total transition triangle power (or time since last transition)
  	prevOnTime = nextClock - transitionTime;
	  powerTotal =  .5 * lastPeakPower * (double)prevOnTime;
  
  	//subtract smaller triangle
  	//Using parametrized line, figure out what instantateous power is
  	chargedLatency = now - transitionTime;
  	
  	totalOffTransitionTime += chargedLatency;
  	
  	currentPower = onPower - (onPower/((double)prevOnTime))*(double)((offLatency - prevOnTime) + chargedLatency); //instantaneous Power
  	
  	
  	//Only add power that has been consumed
  	powerNotConsumed = .5 * currentPower * (double)(prevOnTime - chargedLatency);
  
  	//Subtract larger triangle from smaller one
  	totalPower += (powerTotal - powerNotConsumed);
  	assert((powerTotal - powerNotConsumed) >= 0);
  	//calculate transition point, and new latency
  	transitionPoint = currentPower*((double)onLatency/onPower);
  	

    // it is not fully turned on.
    nextClock=now + (onLatency - (tick_t)transitionPoint);
    
    transitionTime = now;
    lastPeakPower = currentPower;
    assert(nextClock > now);
    
    #if DEBUG
    	cout<<now<<": "<<getName()<<" OFF_TRANSITION -> ON_TRANSITION"<<endl;
    #endif 
  }
  status = FUS_ON_TRANSITION;
  return nextClock;
}



void FunctionalUnit::synchronize(const tick_t &now)
{
   
  
  if(status == FUS_ON_TRANSITION && nextClock <= now)
 	{
    status = FUS_ON; 
  	onTime = now;

  	
  	//If the transition time was less than a full onLatency (i.e. there was a transition to off then a transition to on)
  	if(now - transitionTime < onLatency)
  	{
  		totalOnTransitionTime += (now - transitionTime);
  	
  		//Area under curve for total transition to on
  		power_t powerTotal = .5 * onPower * (double)onLatency;
  		
  		//Find point where the off to on transition occured
  		double transitionPower = (onPower/(double)onLatency) * (double)(onLatency - transitionTime); //Energy at transition point
  		
  		//Find the total area of the triange for power that was not consumed
  		power_t powerNotConsumed = .5 * transitionPower * (double)(onLatency - transitionTime);
  		
  		totalPower += (powerTotal - powerNotConsumed);
  		
  	  assert((powerTotal - powerNotConsumed) >= 0);
  	  #if DEBUG
    		cout<<now<<": "<<getName()<<" ON_TRANSITION -> ON with partial transition"<<endl;
    	#endif 
  	
  	}
  	else
  	{
  	
  		totalOnTransitionTime += onLatency;
  		
  		//full triangle
  		totalPower += .5 * onPower * (double)onLatency;
  	  #if DEBUG
    		cout<<now<<": "<<getName()<<" ON_TRANSITION -> ON with full transition"<<endl;
    	#endif 
  	
  	}
  	totalOnTransitionTime += (nextClock - transitionTime);
  	transitionTime = nextClock;
  	lastPeakPower = onPower;
  	return;
  }
  
  if(status == FUS_OFF_TRANSITION && nextClock <= now)
  {
    status=FUS_OFF; 
  	
  	//first check to see if an entire transition time occurred
  	if(now - transitionTime < offLatency)
  	{
  		//Just add smaller triangle  		
  		double transitionPower = onPower - (onPower/(double)offLatency) * (double)(offLatency - transitionTime);
  		
  		totalPower += .5 * transitionPower * (double)transitionTime;
  	  #if DEBUG
    		cout<<now<<": "<<getName()<<" OFF_TRANSITION -> OFF with partial transition"<<endl;
    	#endif   		
  	
  	}
  	else
  	{
   		//full triangle
  		totalPower += .5 * onPower * (double)onLatency; 	
  		
   	  #if DEBUG
    		cout<<now<<": "<<getName()<<" OFF_TRANSITION -> OFF with full transition"<<endl;
    	#endif 
  	}		
		totalOffTransitionTime += (nextClock - transitionTime);
		transitionTime = nextClock;
		lastPeakPower = 0;
		return;
	}

	
}

void FunctionalUnitManager::synchronize(const tick_t &now)
{
  if(now==globalClock)
    return;
    
  for( unsigned int i=0 ; i<functionalUnits.size() ; i++ )
  {
  	functionalUnits[i]->synchronize(now);
  
  }
  
  globalClock = now;
}

power_t FunctionalUnitManager::getTotalPower(const tick_t &now)
{
	double totalPower = 0;
	for(int i = 0; i < functionalUnits.size(); i++)
		totalPower += functionalUnits[i]->getTotalPower(now);
	
	return totalPower;
}

int FunctionalUnitManager::readFunctionalUnitFile(const char * filename)
{
	
  ifstream infile;
  
  string line;
		
  infile.open(filename);
  if(!infile)
    return -1;

  while(!infile.eof())
  {
  	getline(infile, line);
  
 		stringstream ss(line);
 		
  	vector<string> tokens;
  	
  	string token;
  	while(getline(ss, token, ' ') )
  	{
  		tokens.push_back(token);
  	}

		if(tokens.size() != 4 || tokens[0].at(0) == '#')
			continue;

		//convert from string to appropriate type
		string name = tokens[0];
		tick_t onLatency, offLatency;
		power_t onPower;
		
		stringstream op(tokens[1]);
		stringstream offl(tokens[2]);
		stringstream onl(tokens[3]);
		op >> onPower;
		offl >> offLatency;
		onl >> onLatency;
		
    functionalUnits.push_back(new FunctionalUnit(name, onLatency, offLatency, onPower));
  }
  infile.close();

  return 0;
}

void FunctionalUnitManager::dumpFunctionalUnits()
{
  printf("Printing functional units\n");
  for( unsigned int i=0 ; i<functionalUnits.size() ; i++ )
  {
		functionalUnits[i]->printFU();
  }
  printf("Done printing functional units\n");
}

void FunctionalUnitManager::dumpStats(const tick_t &now)
{
  for( unsigned int i=0 ; i<functionalUnits.size() ; i++ )
  {
  	FunctionalUnit* FU = functionalUnits[i];
		cout<<"\t"
				<<FU->getName()<<": TotalPower("<<FU->getTotalPower(now)
				<<") TotalOnTime("<<FU->getTimeSpentOn(now)
				<<") TotalOffTime("<<FU->getTimeSpentOff(now)
				<<") TotalOnTransitionTime("<<FU->getTimeInOnTransition()
				<<") TotalOffTransitionTime("<<FU->getTimeInOffTransition()<<")"
				<<endl;
  }

}
#undef LOG2
#undef EPSILON
