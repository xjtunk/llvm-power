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
    functionalUnits[unitNumber].nextClock=now+functionalUnits[unitNumber].offLatency;
  }
  else if(functionalUnits[unitNumber].status==FUS_ON_TRANSITION)
  {
    // FIXME: Currently suffers all the latency for turning off, though
    // it is not fully turned on.
    functionalUnits[unitNumber].nextClock=now+functionalUnits[unitNumber].offLatency;
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
    functionalUnits[unitNumber].nextClock=now+functionalUnits[unitNumber].offLatency;
  }
  else if(functionalUnits[unitNumber].status==FUS_OFF_TRANSITION)
  {
    // FIXME: Currently suffers all the latency for turning off, though
    // it is not fully turned on.
    functionalUnits[unitNumber].nextClock=now+functionalUnits[unitNumber].offLatency;
  }
  functionalUnits[unitNumber].status=FUS_ON_TRANSITION;
  return functionalUnits[unitNumber].nextClock;
}

void FunctionalUnitManager::synchronize(const unsigned long &unitNumber, const unsigned long long &now)
{
  assert(unitNumber<functionalUnits.size());
  if(now==globalClock)
    return;
  if(functionalUnits[unitNumber].status==FUS_ON_TRANSITION &&
      functionalUnits[unitNumber].nextClock<=now)
    functionalUnits[unitNumber].status=FUS_ON; 
  if(functionalUnits[unitNumber].status==FUS_OFF_TRANSITION &&
      functionalUnits[unitNumber].nextClock<=now)
    functionalUnits[unitNumber].status=FUS_OFF; 
}

void FunctionalUnitManager::synchronize(const unsigned long long &now)
{
  if(now==globalClock)
    return;
  for( unsigned int i=0 ; i<functionalUnits.size() ; i++ )
  {
    if(functionalUnits[i].status==FUS_ON_TRANSITION &&
       functionalUnits[i].nextClock<=now)
     functionalUnits[i].status=FUS_ON; 
    if(functionalUnits[i].status==FUS_OFF_TRANSITION &&
       functionalUnits[i].nextClock<=now)
     functionalUnits[i].status=FUS_ON; 
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
