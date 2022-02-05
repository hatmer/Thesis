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
#include <stdlib.h>

#define LOG_MODULE "App"
#define LOG_LEVEL LOG_LEVEL_INFO
//#include "net/ipv6/simple-udp.h"


// capacitor full amt of energy, amt. of energy used by main loop, amt. energy gained in full sunlight

/*
double LOOP_ENERGY_USE = 10.0;

double attackTotal[10] = {0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0}; // all energy blocked
double attackGreenhouse[10] = {5.0, 5.0, 5.0, 5.0, 5.0, 5.0, 5.0, 5.0, 5.0, 5.0}; //gardner blocks 75% of energy
double attackSatellite[10] = {5.0, 5.0, 5.0, 1.0, 0.0, 1.0, 5.0, 5.0, 5.0}; // satellite blocks the sun
double FSE = 20.0; // full sun energy
*/
struct State {
  double wakePercent;             // wakePercent >= QoS
  double energy;                  // used to measure how much energy is harvested during an attack
  int attackTimeIdx;
};
/*
struct Buffer {
  const void ** buffer;       // only used by network device
  int bufferHead;             // points to head of ring.buffer
  int bufferTail;             // points to tail of ring.buffer
};


struct State state = {.wakePercent = .25, .energy = 100.0, .attackTimeIdx = 0};
struct Buffer ring = {.bufferHead = 0, .bufferTail = 0};
*/
/**
 * Proceedures
 */
/*
void
init()
{
  ring.buffer = malloc(sizeof(void*)*BUFFERSIZE);
  return;
}
*/
/**
 * Get the time in seconds since some arbitrary point. Used for high
 * precision timing measurements.
 */

int
get_time()
{
   int time = abs((int)clock_seconds());
  //LOG_INFO("get_time: %d\n", time);
   return time;
}

/*
 * Get the energy percentage from the energy buffer
 * TODO implement
 *
 * energy attack simulation:
 * harvested energy simulation: get_energy returns deterministic energy increases based on attack scenario
 *
 * attack scenario: time window (idx * x second periods) + amount of energy gained each round + attack_detection_flag
 *
 * 1. get capacitor full charge amt
 * 2. calculate main loop energy cost
 * 3. decrement energy lvl each main loop round
 * 4. if attack ongoing: increase energy lvl according to attack scenario
 *    else: increase energy lvl by fixed amt (full sunlight)
 *
 *    need: capacitor full amt of energy, amt. of energy used by main loop, amt. energy gained in full sunlight
 */


/*
static double
get_energy(struct Miti *userVars)
{
  // update energy level
  if (userVars->attack) {
    state.energy += attackTotal[state.attackTimeIdx % 10]; // TODO use different attacks
    state.attackTimeIdx++;
  } else {
    state.energy += FSE;
    state.attackTimeIdx = 0; // reset
  }
  return state.energy;

}
*/
/*
 * Determine if device should be in wake mode
 *
 */
/*
static void
conserve_energy(struct Miti *userVars)
{
  LOG_INFO("checking if should be asleep\n");
  //static struct etimer sleep_timer;
  int t = get_time();
  printf("time is %d\n", t);
  printf("test: %d\n", ((t % 100) / 10));

  if ( userVars->attack &&  ((t % 100) / 10) > (state.wakePercent * 10))
  {
    int sleepUntil = t + (10 - ((t % 100) / 10))*1000;
    printf("sleeping until: %d\n", sleepUntil);
    LOG_INFO("sleeping\n");
    //NETSTACK_RADIO.off();
    //SENSORS_DEACTIVATE(userVars->pir_sensor);

    // sleep until next wake period
    printf("busy sleep for %d\n", sleepUntil - get_time());
    while (get_time() < sleepUntil){}

    LOG_INFO("waking up\n");
    //SENSORS_ACTIVATE(userVars->pir_sensor);
    //NETSTACK_RADIO.on();
  }
}
*/

/*
 * Update QoS (wakePercent)
 */
/*
static void
AIMD(struct Miti *userVars)
{
  double energyConsumed = state.energy - get_energy(userVars);
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
}
*/

/**
 * Wrap a network device
 */
/*
int
send_wrapper(int (*send)(simple_udp_connection*, const void*, uint16_t, const uip_ipaddr_t*), simple_udp_connection* udp_conn, const void* str, uint16_t length, const uip_ipaddr_t*dest_ipaddr, Miti *userVars)
{
  // 1. sleep mode
//  conserve_energy(userVars);

  // 2. wake mode
  //send(udp_conn, str, length, dest_ipaddr);

  // 3. AIMD adjustment to state.wakePercent
  AIMD(userVars);

  return 0;
}
*/
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
