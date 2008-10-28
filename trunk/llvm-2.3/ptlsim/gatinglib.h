#ifndef  __GATINGLIB_H__
#define  __GATINGLIB_H__

// copied from PTLSim
#include  <globals.h>
#include  <superstl.h>

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
struct FunctionalUnit
{
  // Current status of the functional unit.
  FunctionalUnitStatus status;
  // name.
  stringbuf name;
  // power that it consumes if fully on (peak power)
  double onPower;
  unsigned long long offLatency;
  unsigned long long onLatency;
  // the next synchronization event takes place at this clock cycle.
  unsigned long long nextClock;
  
  //Time since transition from OnTransition to On
  unsigned long long onTime;
  
  //Time since last transition (use to calculate area under curve)
  unsigned long long transitionTime;
  
  //Store this for conditions where an on transition is interrupted by an off transition or vice versa
  double lastPeakPower;
  
  //running total of power
  double totalPower;

  FunctionalUnit()
  {
    // turn on every functional unit by default.
    status=FUS_ON;
  }
  
  inline double getTotalPower()
  {
  	return totalPower;
  }
};

struct FunctionalUnitManager
{

  // The last clock that was synchronized
  unsigned long long globalClock;

  // This is the vector of functional units.
  dynarray<FunctionalUnit> functionalUnits;
  // Gate all the functional units with the mask.
  void gateWithMask(const unsigned long long &mask, const unsigned long long &now);
  // Query function to see if functional unit is available.
  bool functionalUnitAvailable(const unsigned long &unitNumber, const unsigned long long &now);
  // turn off a functional unit.
  unsigned long long turnOff(const unsigned long &unitNumber, const unsigned long long &now);
  // turn on a functional unit.
  unsigned long long turnOn(const unsigned long &unitNumber, const unsigned long long &now);
  // synchronization function for the entire functional unit vector
  void synchronize(const unsigned long long &now);
  void synchronize(const unsigned long &unitNumber, const unsigned long long &now);

	double getTotalPower();
  // IO
  // Read an input (usually from fu.txt, and create the functional unit vector.
  int readFunctionalUnitFile(const char * filename);
  // for debugging functional units' status.
  void dumpFunctionalUnits();
};


#endif  /*__GATINGLIB_H__*/
