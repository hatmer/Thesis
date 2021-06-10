/**
 * Attack Mitigation system for energy harvesting intermittent devices
 * 
 * Author: Hannah Atmer <email>
 *
 */

#include <unistd.h>
#include <sys/time.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

/* Tune this value: amount for additive increase */
#define DELTA 2 

/**
 * Proceedures
 */

/**
 * Get the time in seconds since some arbitrary point. Used for high
 * precision timing measurements.
 */
static double
get_time()
{
        struct timeval tv;

        if (gettimeofday(&tv, NULL)) {
                fprintf(stderr, "gettimeofday failed. Aborting.\n");
                abort();
        }
        return tv.tv_sec + tv.tv_usec * 1E-6;
}

/*
 * Get the energy percentage from the energy buffer
 *
 */
static double
get_energy()
{
  return 100;
}

/*
 * Determine if device should be in wake mode
 *
 */
static bool
should_be_awake(*vars)
{
  return !vars.attack || (get_time() % 10) < vars.wakePercent* 10;
}

/**
 * Wrap a network device
 */
static int
send_wrapper(int (*send)(*vars, id, **args))
{
  // send everything in buffer first
  while (should_be_awake(vars)) {
    if (vars.bufferHead != vars.bufferTail) {
      send(vars->buffer[vars.bufferHead]);
      vars.bufferHead += 1;
    }
  }
  // handle message
  if (should_be_awake(vars)) {
    send(args->message);
  } else {
    // check if buffer is full
    if (vars.bufferTail+1 % sizeof(vars->buffer)/sizeof(vars->buffer[0])) {
      return -1; // error code here
    }
    vars->buffer[vars.bufferTail] = args->message;
    vars.bufferTail += 1;
  }

  return 0;
}


/* 
 * Wrap a sensor device
 */
static void*
sense_wrapper(void* (*sense)(*vars, id, **args))
{
  if (should_be_awake(vars)) {
      void* dataPtr = sense();
  } else {
    return (void*) -1; // error code here
  }
  return dataPtr;
}

/*
 * Update QoS (wakePercent)
 */
static void
AIMD(*vars)
{
  double energyNow = get_energy();
  double energyConsumed = vars.energy - energyNow;
  if (energyConsumed < 0) { // gained energy: increase wakePercent
    if (vars.wakePercent + DELTA <= 100) {
      vars.wakePercent += DELTA; 
    } else{
      vars.wakePercent = 100;
    }
  } else { // lost energy: decrease wakePercent, with minimum wakePercent = vars.QoS
    vars.wakePercent /= 2;
    if (vars.wakePercent < vars.QoS) {
      vars.wakePercent = vars.QoS;
    }
  }
  return;
}

int
main(int argc, char *argv[])
{
        return 0;
}

