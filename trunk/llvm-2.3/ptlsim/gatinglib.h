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
  FunctionalUnitStatus status;
  stringbuf name;
  double onPower;
  unsigned long long offLatency;
  unsigned long long onLatency;
  unsigned long long nextClock;
  FunctionalUnit()
  {
    // turn on every functional unit by default.
    status=FUS_ON;
  }
};

struct FunctionalUnitManager
{
  double totalPower;
  unsigned long long globalClock;

  dynarray<FunctionalUnit> functionalUnits;
  void gateWithMask(const unsigned long long &mask, const unsigned long long &now);
  bool functionalUnitAvailable(const unsigned long &unitNumber, const unsigned long long &now);
  unsigned long long turnOff(const unsigned long &unitNumber, const unsigned long long &now);
  unsigned long long turnOn(const unsigned long &unitNumber, const unsigned long long &now);
  // synchronization function
  void synchronize(const unsigned long long &now);
  void synchronize(const unsigned long &unitNumber, const unsigned long long &now);

  // IO
  int readFunctionalUnitFile(const char * filename);
  void dumpFunctionalUnits();
};


#endif  /*__GATINGLIB_H__*/
