CC=llvm-gcc
CFLAGS=-emit-llvm
LDFLAGS=
SOURCES=tracetest.cpp tracetest1.cpp loop.cpp manycalls.cpp
OBJECTS=$(SOURCES:.cpp=.bc)
GRAPH_OBJECTS=$(SOURCES:.cpp=.ps)
###EXECUTABLE=hello

###all: $(SOURCES) $(EXECUTABLE)
all: $(OBJECTS) 
	  
###$(EXECUTABLE): $(OBJECTS) 
###	  $(CC) $(LDFLAGS) $(OBJECTS) -o $@

%.bc: %.cpp
	  $(CC) $(CFLAGS) -c $< -o $@

%.ps: %.bc
	opt -load ../../Debug/lib/profile_rt.so -profile-loader -print-cfg $<
	dot -Tps -o $@ cfg.main.dot
	rm cfg.main.dot

graph: $(GRAPH_OBJECTS)

###tracetest.bc: tracetest.cpp
###	$(CC) $(CFLAGS) -c tracetest.cpp -o tracetest.bc

clean:
	rm -f *.bc *.ll *.o *.dot *.ps

	

