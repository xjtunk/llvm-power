/*===-- EdgeProfiling.c - Support library for edge profiling --------------===*\
|*
|*                     The LLVM Compiler Infrastructure
|*
|* This file is distributed under the University of Illinois Open Source      
|* License. See LICENSE.TXT for details.                                      
|* 
|*===----------------------------------------------------------------------===*|
|* 
|* This file implements the call back routines for the edge profiling
|* instrumentation pass.  This should be used with the -insert-edge-profiling
|* LLVM pass.
|*
\*===----------------------------------------------------------------------===*/

#include "Profiling.h"
#include <stdlib.h>
#include <signal.h>
#include <stdio.h>
#include <unistd.h>

static unsigned *ArrayStart;
static unsigned NumElements;


/* EdgeProfAtExitHandler - When the program exits, just write out the profiling
 * data.
 */
static void EdgeProfAtExitHandler() {
  /* Note that if this were doing something more intelligent with the
   * instrumentation, we could do some computation here to expand what we
   * collected into simple edge profiles.  Since we directly count each edge, we
   * just write out all of the counters directly.
   */
  write_profiling_data(EdgeInfo, ArrayStart, NumElements);
}

// Added by Brooks
// Function to handle timer interrupts
struct sigaction signalaction;
void handleSIGALRM(int arg) {
  printf("Handling SIGALRM\n");
  EdgeProfAtExitHandler();	
  alarm(1);
}

/* llvm_start_edge_profiling - This is the main entry point of the edge
 * profiling library.  It is responsible for setting up the atexit handler.
 */
int llvm_start_edge_profiling(int argc, const char **argv,
                              unsigned *arrayStart, unsigned numElements) {
  int Ret = save_arguments(argc, argv);
  ArrayStart = arrayStart;
  NumElements = numElements;
  // Brooks
  // Added signal handler for ALARM to do timer interrupts
  // They trigger the edge profiler to run
  atexit(EdgeProfAtExitHandler);
#if 0
  sigemptyset(&signalaction.sa_mask);
  signalaction.sa_flags = SA_RESTART;
  signalaction.sa_handler = &handleSIGALRM;
  sigaction(SIGALRM, &signalaction, NULL);
  alarm(1);
#endif
  return Ret;
}

