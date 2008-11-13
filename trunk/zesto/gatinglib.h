#ifndef  __GATINGLIB_H__
#define  __GATINGLIB_H__

// copied from PTLSim
#include <fstream>
#include <iostream>
#include <vector>
#include <string>
#include <sstream>


typedef unsigned long long tick_t;
typedef unsigned int mask_t;
typedef double power_t;


#define DEBUG 1
using namespace std;

// Not being used at the moment, but will likely be useful.
enum FunctionalUnitType
{
  FUT_INT_ADDER,
  FUT_INT_DIVIDER,
  FUT_BALL
};

// Current status of the functional units.
enum FunctionalUnitStatus
{
  FUS_OFF,
  FUS_OFF_TRANSITION,
  FUS_ON_TRANSITION,
  FUS_ON
};

// Each functional unit has a name, onPower, current status, next clock, etc.
class FunctionalUnit
{
public:
	FunctionalUnit(string& _name, tick_t _onLatency, tick_t _offLatency, power_t _onPower)
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
		}


	//Interface Methods
	tick_t turnOn(const tick_t &now);
	tick_t turnOff(const tick_t &now);
	void synchronize(const tick_t &now);
	void updateInPipeline() {decodeStatus = true;}
	void updateNotInPipeline() {decodeStatus = false;}
	bool getUpdateStatus() {return decodeStatus;}
	
	bool checkAvailable() {return status == FUS_ON;}
	
	//stats
	string getName() {return name; }
	
	power_t getTotalPower(const time_t &now) 
	{
		synchronize(now);
		power_t powerSinceLastOn = 0;
		if(status == FUS_ON)//calcuate time spent on
		{
			powerSinceLastOn = onPower * (now - transitionTime);
		}
		return totalPower + powerSinceLastOn;
	}//dont forget about time spent on
	
	
	
	tick_t getTimeSpentOn(const tick_t &now) {
		synchronize(now);
		if(status == FUS_ON)
			return totalOnTime + (now - onTime);
		else
			return totalOnTime;
		
	}
	tick_t getTimeSpentOff(const tick_t &now) 
	{
		synchronize(now);
		if(status == FUS_OFF)
			return totalOffTime + (now - transitionTime);
		else
			return totalOffTime;
	
	}
	
	tick_t getTimeInOnTransition() {return totalOnTransitionTime;}
	tick_t getTimeInOffTransition() {return totalOffTransitionTime;}
	
	//debug
	void printFU()
	{
		cout<<name<<": onPower("<<onPower<<") offLatency("<<offLatency<<") onLatency("<<onLatency<<")"<<endl;
	
	}
private:
	//constant attributes
	string name;
  power_t onPower; // power that it consumes if fully on (peak power)
  tick_t offLatency;
  tick_t onLatency;	
	
	
	//state attributes
  FunctionalUnitStatus status;
	tick_t nextClock;// the next synchronization event takes place at this clock cycle.
 	tick_t onTime; // time that transition to on was completed
	tick_t transitionTime;//Time since last transition (use to calculate area under curve)
  bool decodeStatus;
  
  power_t lastPeakPower;  //highest power level achieved during transition (used for overlapping on/off transitions)
 
 	//stats
 	power_t totalPower;
  tick_t totalOnTime;
  tick_t totalOffTime;
  tick_t totalOnTransitionTime;
  tick_t totalOffTransitionTime;
  
};

class FunctionalUnitManager
{
public:
	FunctionalUnitManager(const char * filename)
	{
		if(!readFunctionalUnitFile(filename))
		{
			globalClock = 0;
			
		}
		else
			cerr<<"Error reading file "<<filename<<endl;
	
	}

	//Processor Interface
	void processAtIssue(const mask_t &mask, const tick_t &now);
	void processAtCommit(const mask_t &mask, const tick_t &now);
	void processAtFlush(const mask_t &mask, const tick_t &now);
	
	//Stats
	power_t getTotalPower(const tick_t &now);


  //Debug
  void dumpFunctionalUnits();
  void dumpStats(const tick_t &now);
  bool functionalUnitAvailable(const unsigned long &unitNumber, const tick_t &now);
  
private:
  // The last clock that was synchronized
  tick_t globalClock;

  // This is the vector of functional units.
  vector<FunctionalUnit*> functionalUnits;
 
  
  //Maintainance Functions
  // turn off a functional unit.
  tick_t turnOff(const unsigned long &unitNumber, const tick_t &now);
  // turn on a functional unit.
  tick_t turnOn(const unsigned long &unitNumber, const tick_t &now);
  // synchronization function for the entire functional unit vector
  void synchronize(const tick_t &now);
  void synchronize(const unsigned long &unitNumber, const tick_t &now);

	
  // IO
  // Read an input (usually from fu.txt, and create the functional unit vector.
  int readFunctionalUnitFile(const char * filename);

};


#endif  /*__GATINGLIB_H__*/
