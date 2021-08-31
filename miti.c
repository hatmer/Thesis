/**
 * Attack Mitigation system for energy harvesting intermittent devices
 * 
 * Author: Hannah Atmer <email>
 *
 */

//#include <unistd.h>
//#include <sys/time.h>
//#include <time.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "miti.h"
//#include "net/ipv6/simple-udp.h"

//typedef struct simple_udp_connection simple_udp_connection

/* Tune this value: amount for additive increase */
#define DELTA 2 
/* Tune this value: amount of space in buffer */
#define BUFFERSIZE 32



struct State {
  double wakePercent;             // wakePercent >= QoS
  double energy;                  // used to measure how much energy is harvested during an attack
};

struct Buffer {
  const void ** buffer;       // only used by network device
  int bufferHead;             // points to head of ring.buffer
  int bufferTail;             // points to tail of ring.buffer
};


struct State state = {.wakePercent = .25, .energy = 0.0};
struct Buffer ring = {.bufferHead = 0, .bufferTail = 0};

/**
 * Proceedures
 */

void
init()
{
  ring.buffer = malloc(sizeof(void*)*BUFFERSIZE);
  return;
}

/**
 * Get the time in seconds since some arbitrary point. Used for high
 * precision timing measurements.
 */

static int
get_time()
{
  int time = (int)clock_seconds();
  printf("%d", time);
   return time;
}

/*
 * Get the energy percentage from the energy buffer
 * TODO implement
 *
 */
/*
static double
get_energy()
{
  return 100;
}*/

/*
 * Determine if device should be in wake mode
 *
 */
static int
should_be_awake(struct Miti *userVars)
{
  return /*!userVars->attack ||*/ (get_time() % 10 < state.wakePercent * 10);
}

/**
 * Wrap a network device
 */
int
send_wrapper(int (*send)(simple_udp_connection*, const void*, uint16_t, const uip_ipaddr_t*), simple_udp_connection* udp_conn, const void* str, uint16_t length, const uip_ipaddr_t*dest_ipaddr, Miti *userVars)
{
  // send everything in buffer first
  while (should_be_awake(userVars)) {
    if (ring.bufferHead != ring.bufferTail) { // TODO double-check that ring buffer works correctly
      send(udp_conn, ring.buffer[ring.bufferHead], length, dest_ipaddr ); // TODO must store correct length somewhere
      ring.bufferHead += 1;
    }
  }
  // handle message
  if (should_be_awake(userVars)) {
    send(udp_conn, str, length, dest_ipaddr );
  } else {
    // check if buffer is full
    if (ring.bufferTail+1 % sizeof(ring.buffer)/sizeof(ring.buffer[0])) {
      return -1; // error code here
    }
    
    ring.buffer[ring.bufferTail] = str; // pointer to string
    ring.bufferTail += 1;
  }
  return 0;
}

/* 
 * Wrap a sensor device
 */
/*
static void*
sense_wrapper(void* (*sense)(struct *vars, **args))
{
  if (should_be_awake(userVars)) {
      void* dataPtr = sense();
  } else {
    return (void*) -1; // error code here
  }
  return dataPtr;
}
*/

/*
 * Update QoS (wakePercent)
 */
/*
static void
AIMD(struct Miti *userVars)
{
  double energyConsumed = state.energy - get_energy();
  if (energyConsumed < 0) { // gained energy: increase wakePercent
    if (state.wakePercent + DELTA <= 100) {
      state.wakePercent += DELTA; 
    } else{
      state.wakePercent = 100;
    }
  } else { // lost energy: decrease wakePercent, with minimum wakePercent = vars.QoS
    state.wakePercent /= 2;
    if (state.wakePercent < userVars->QoS) {
      state.wakePercent = userVars->QoS;
    }
  }
  return;
}*/
