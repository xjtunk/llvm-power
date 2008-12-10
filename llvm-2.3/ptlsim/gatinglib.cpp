#include  <gatinglib.h>
//#include <cassert>

// Useful functions for converting a bitmask into individual functional unit indices.
#define LOG2(x) (ffs(x)-1)

// Useful for comparing doubles. Instead of double d==double a, use abs(d-a)<EPSILON.
// Because we are trying to model unlimited precision using limited number of bits.
#define EPSILON 1e-5

FunctionalUnit::FunctionalUnit(char* _name, tick_t _onLatency, tick_t _offLatency, power_t _onPower)
		: name(_name), onPower(_onPower), offLatency(_offLatency), onLatency(_onLatency)
{
	//state attributes
	status=FUS_ON;
	nextClock = 0;
	onTime = 0;
	transitionTime = 0;
	
	//stats
	totalPower = 0;
	totalOnTime = 0;
	totalOffTime = 0;
	totalOnTransitionTime = 0;
	totalOffTransitionTime = 0;
	
	decodeStatus = false;
	valid = false;
}
FunctionalUnit::FunctionalUnit(char* _name, tick_t _onLatency, tick_t _offLatency, power_t _onPower, bool _valid)
		: name(_name), onPower(_onPower), offLatency(_offLatency), onLatency(_onLatency)
{
	//state attributes
	status=FUS_ON;
	nextClock = 0;
	onTime = 0;
	transitionTime = 0;
	
	//stats
	totalPower = 0;
	totalOnTime = 0;
	totalOffTime = 0;
	totalOnTransitionTime = 0;
	totalOffTransitionTime = 0;
	
	decodeStatus = false;
	valid = _valid;
}
void FunctionalUnitManager::processAtIssue(const mask_t &mask, const tick_t &now)
{
	int i;
	mask_t _mask = mask >> 1; 
	tick_t realTime = now - offset;
	for(i = 0; i < functionalUnits.size(); i++)
	{
	
		if(_mask & 0x1)
		{
			cerr<<"Turning on functional unit "<<functionalUnits[i]->getName()<<endl;
			functionalUnits[i]->turnOn(realTime);	
		//	functionalUnits[i]->updateInPipeline(); //indicate that a turnon has been dispatched but not commited
		}
		else
		{
			cerr<<"Turning off functional unit "<<functionalUnits[i]->getName()<<endl;
			functionalUnits[i]->turnOff(realTime);
		}
		_mask >>= 1;
	}
	
	
}

void FunctionalUnitManager::initializationComplete(const tick_t &now)
{
	offset = now;
	initializationTime = now;

}

void FunctionalUnitManager::processAtCommit(const mask_t &mask, const tick_t &now)
{
	int i;
	mask_t _mask = mask;
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

void FunctionalUnitManager::processAtFlush(const mask_t &mask, const tick_t &now)
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
  	totalPower += onPower * (double)(now - onTime);
  	transitionTime = now;
    nextClock=now + offLatency;
    lastPeakPower = onPower;
    
    
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
  	totalOnTransitionTime += now - lastTransition;
  	
  	//Calculate energy right now
  	double currentPower = (onPower/(double)onLatency)*(double)(chargedLatency + lastTransition);
  	
  	
  	//Only add power that has been consumed
  	power_t powerConsumed = .5 * currentPower * (double)chargedLatency;
  	power_t powerNotConsumed = .5 * lastPeakPower * lastTransition;
  	
  	totalPower +=  (powerConsumed - powerNotConsumed);
  	
  	//assert((powerConsumed - powerNotConsumed) >= 0);
    
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
  
  valid = true;
  if(status == FUS_ON)
  {
    #if DEBUG
    	cout<<now<<": "<<getName()<<" ON -> ON"<<endl;
    #endif 
  	return 0;
  } 
  else if(status == FUS_OFF)
  {
  //update stats
    totalOffTime += (now - transitionTime);
  	transitionTime = now;
    nextClock=now + onLatency;
    lastPeakPower = 0; 
    
    
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
  	
  	totalOffTransitionTime += offLatency - prevOnTime;
  	
  	currentPower = onPower - (onPower/((double)prevOnTime))*(double)((offLatency - prevOnTime) + chargedLatency); //instantaneous Power
  	
  	
  	//Only add power that has been consumed
  	powerNotConsumed = .5 * currentPower * (double)(prevOnTime - chargedLatency);
  
  	//Subtract larger triangle from smaller one
  	totalPower += (powerTotal - powerNotConsumed);
  	//assert((powerTotal - powerNotConsumed) >= 0);
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

FunctionalUnitManager::FunctionalUnitManager(const char * filename)
	{
    functionalUnits.push(new FunctionalUnit("FUT_INT_ADD_ARITH", 10, 8, .5, 1));
    functionalUnits.push(new FunctionalUnit("FUT_INT_ADD_LOGIC", 7, 5, .3, 1));
    functionalUnits.push(new FunctionalUnit("FUT_INT_SHIFT", 6, 3, .2, 1));
    functionalUnits.push(new FunctionalUnit("FUT_INT_MUL", 30, 25, 2, 1));
    functionalUnits.push(new FunctionalUnit("FUT_INT_DIV", 35, 33, 2));
    functionalUnits.push(new FunctionalUnit("FUT_FP_ADD", 20, 18, 1.3));
    functionalUnits.push(new FunctionalUnit("FUT_FP_MUL", 45, 40, 1.5));
    functionalUnits.push(new FunctionalUnit("FUT_FP_DIV", 60, 50, 1.8));
    functionalUnits.push(new FunctionalUnit("FUT_FP_SQRT", 60, 50, 1.8));
    functionalUnits.push(new FunctionalUnit("FUT_LOAD", 18,15, .8,1));
    functionalUnits.push(new FunctionalUnit("FUT_STORE",18,15, .8,1));
    functionalUnits.push(new FunctionalUnit("FUT_AGU", 8, 5, .2,1));
		functionalUnits.push(new FunctionalUnit("FUT_BRANCH", 25, 20, .95,1));
		functionalUnits.push(new FunctionalUnit("FUT_MOVE", 6, 3, .2,1));
		functionalUnits.push(new FunctionalUnit("FUT_SET", 5, 2, .2,1));
		functionalUnits.push(new FunctionalUnit("FUT_CMP", 10,8, .6,1));
    functionalUnits.push(new FunctionalUnit("FUT_L1_DCACHE", 30, 26, 1.2));
    functionalUnits.push(new FunctionalUnit("FUT_L2_UCACHE", 60, 55, 2.0));
		functionalUnits.push(new FunctionalUnit("FUT_MMX", 70, 65, 5.0));
		functionalUnits.push(new FunctionalUnit("FUT_VECTOR_ALU", 40, 35, 1.5));
		functionalUnits.push(new FunctionalUnit("FUT_VECTOR_MUL", 60, 55, 2.1));
		functionalUnits.push(new FunctionalUnit("FUT_VECTOR", 60, 55, 2.5));


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
	
#if 0
  unsigned int size=0;
  stringbuf sb;
  istream infile;
  dynarray<char*> tokens;

  infile.open(filename);
  if(!infile)
  {
  	cerr<<"Error reading file!"<<endl;
    return -1;
	}
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
    functionalUnits[size-1] = new FunctionalUnit(tokens[0], atoll(tokens[2]),atoll(tokens[3]), atof(tokens[1]));
  }
  infile.close();
#endif


  return 0;
}

void FunctionalUnitManager::dumpFunctionalUnits()
{
  cerr<<"Printing functional units"<<endl;
  for( unsigned int i=0 ; i<functionalUnits.size() ; i++ )
  {
		functionalUnits[i]->printFU();
  }
  cerr<<"Done printing functional units\n"<<endl;
}

void FunctionalUnitManager::dumpStats(const tick_t &now)
{
//	cerr<<"functionalUnits.size: "<<functionalUnits.size()<<" now: "<<now<<endl,flush;
	stringbuf sb;
	double totalPower = 0;
	double totalPowerNoOptimized = 0;
	tick_t realTime = now - offset;
  for( unsigned int i=0 ; i<functionalUnits.size() ; i++ )
  {
  	FunctionalUnit* FU = functionalUnits[i];
  	FU->synchronize(now);
  	if(FU->isValid())
  	{
			sb<<"Functional Unit: ",					FU->getName(), endl;
			sb<<"\tTotalPower: ",							FU->getTotalPower(realTime), endl;
			sb<<"\tTotalOnTime: ",						FU->getTimeSpentOn(realTime), endl;
			sb<<"\tTotalOffTime:",						FU->getTimeSpentOff(realTime), endl;
			sb<<"\tTotalOnTransitionTime: ",	FU->getTimeInOnTransition(), endl;
			sb<<"\tTotalOffTransitionTime: ",	FU->getTimeInOffTransition(), endl;
			sb<<"\tPercent Time On: ",				(double)(FU->getTimeSpentOn(realTime)/(double)realTime), endl;
			
			
			totalPower += FU->getTotalPower(now);
			totalPowerNoOptimized += FU->getPowerNotOptimized(now);
  	}
  }
  
  sb<<"Initialization Time: ", initializationTime, endl;
  sb<<"Total Power Consumed: ", totalPower, " in ",realTime,endl;
  sb<<"Total Non-Optimized Power: ", totalPowerNoOptimized, endl;
  sb<<"Percent Power Utlization: ", totalPower/totalPowerNoOptimized*(double)100,endl;
  cerr<<sb, flush;
}
#undef LOG2
#undef EPSILON
