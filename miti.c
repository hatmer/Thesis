/**
 * Attack Mitigation system for energy harvesting intermittent devices
 * 
 * Author: Hannah Atmer <email>
 *
 */

//#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "miti.h"
#include "sys/log.h"
#include "netstack.h"
#include "sensors.h"
#include "pir-sensor.h"

#define LOG_MODULE "App"
#define LOG_LEVEL LOG_LEVEL_INFO
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
  LOG_INFO("get_time: %d\n", time);
   return time;
}

/*
 * Get the energy percentage from the energy buffer
 * TODO implement
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
static void
conserve_energy(struct Miti *userVars)
{
  LOG_INFO("checking if should be asleep\n");
  int t = get_time() % 10;
  if ( /*userVars->attack && */ (t > (state.wakePercent) * 10))
  {
    NETSTACK_RADIO.off();
    SENSORS_DEACTIVATE(userVars->pir_sensor);
//    sleep(10 - t);  // sleep until next wake period
    SENSORS_ACTIVATE(userVars->pir_sensor);
    NETSTACK_RADIO.on();
  }

}

/*
 * Update QoS (wakePercent)
 */

static void
AIMD(struct Miti *userVars)
{
  double energyConsumed = state.energy - get_energy();
  if (energyConsumed < 0) { // gained energy: increase wakePercent
    if (state.wakePercent + DELTA <= 1.0) {
      state.wakePercent += DELTA;
    } else{
      state.wakePercent = 1.0;
    }
  } else { // lost energy: decrease wakePercent, with minimum wakePercent = vars.QoS
    state.wakePercent /= 2;
    if (state.wakePercent < userVars->QoS) {
      state.wakePercent = userVars->QoS;
    }
  }
  return;
}


/**
 * Wrap a network device
 */
int
send_wrapper(int (*send)(simple_udp_connection*, const void*, uint16_t, const uip_ipaddr_t*), simple_udp_connection* udp_conn, const void* str, uint16_t length, const uip_ipaddr_t*dest_ipaddr, Miti *userVars)
{
  // 1. sleep mode
  conserve_energy(userVars);

  // 2. wake mode
  send(udp_conn, str, length, dest_ipaddr);

  // 3. AIMD adjustment to state.wakePercent
  AIMD(userVars);

  return 0;
}

/* 
 * Wrap a sensor device
 */

/*
sense_wrapper(void* (*sense)(struct *vars, **args))
{
  // 1. sleep mode
  conserve_energy(userVars);

  // 2. wait for sens
  

  // 3. adjust wakePercent
  AIMD(userVars);

  return result;
}
*/
