#include "contiki.h"
#include "random.h"
#include <limits.h>
#include "sys/energest.h"
#include "sys/log.h"
#define LOG_MODULE "App"
#define LOG_LEVEL LOG_LEVEL_INFO

#define WITH_SERVER_REPLY  1
#define UDP_CLIENT_PORT	8765
#define UDP_SERVER_PORT	5678

#define SEND_INTERVAL 10  // Application sends a message every X seconds

#define MITI 1

/* Simulation Parameters */
static unsigned long energy = 225;     // energy buffer level
unsigned long starting_energy = 225;
unsigned long harvestable = 1;        // amount of harvestable energy per 10 milliseconds during attack

static unsigned long total_time = 0;
static unsigned long last_cpu_reading, prev_time;


//unsigned long starting_energies[10] = {90, 135, 180, 225}; // u Joules
              // critical level       = {14, 21, 29, 36} uJoules
#ifdef MITI
static bool attack_ongoing = true;
static int off_seconds = 90;
int critical_energy_level = 36;
int delta = 1;
#endif


// energy (u joules) used per 10 milliseconds
unsigned long cpu_s_energy = 12; // amount of energy (mu amps) used by 10 cpu millisecond
unsigned long radio_s_transmit_energy = 275; // amount of energy used by a radio transmit 10 milliseconds
unsigned long radio_s_listen_energy = 275; // amount of energy used by a radio listen 10 milliseconds
//unsigned long lpm_energy = .0045; //.15; // LPM3
unsigned long sensor_reading_energy = 198; // uJoules

static inline unsigned long to_centi_seconds(uint64_t time)
{
  return (unsigned long)(time / (ENERGEST_SECOND / 100));
}

static inline unsigned long to_milli_seconds(uint64_t time)
{
  return (unsigned long)(time / (ENERGEST_SECOND / 1000));
}

/*---------------------------------------------------------------------------*/
PROCESS(udp_client_process, "Demo Application (UDP client)");
AUTOSTART_PROCESSES(&udp_client_process);
/*---------------------------------------------------------------------------*/

void update_energy(bool radio, unsigned long lpm_seconds, bool cpu, bool sensor)
{
  /*********** update energy level *************/
  energest_flush();
  
  //printf("******** Updating Energy ********* \nenergy before update %lu\n", energy);
  if (cpu) {
    energy -= ((to_centi_seconds(energest_type_time(ENERGEST_TYPE_CPU)) - last_cpu_reading - (lpm_seconds*100))*cpu_s_energy);
  }
  printf("cpu: %lu ms\n", (to_centi_seconds(energest_type_time(ENERGEST_TYPE_CPU)) - last_cpu_reading - (lpm_seconds*100))*10);
  last_cpu_reading = to_centi_seconds(energest_type_time(ENERGEST_TYPE_CPU));
  
  if (sensor) {
    // collected sensor data for .3 ms
    energy -= sensor_reading_energy;
  }

  if (radio) {
    // transmitted for 3.3 milliseconds
    energy -= (radio_s_listen_energy / 3);
  }

  //printf("energy used in LPM: %lu\n", (lpm_seconds * 100 * 9 / 2000));
  energy -= (lpm_seconds * 100 * 9 / 2000);

  prev_time = total_time;
  total_time = to_centi_seconds(ENERGEST_GET_TOTAL_TIME());

  
  //printf("energy harvested: %lu\n", ((total_time - prev_time) * harvestable / 1000));


  energy += ((total_time - prev_time) * harvestable / 10 );
  if (energy > starting_energy) {
    if (energy < 400000000) { 
      energy = starting_energy;
    }
  }
 
/*  if (energy > 400000) {
    printf("energy after update: -%lu uJoules\n", ULONG_MAX-energy);
  } else {
    printf("energy after update: %lu uJoules\n",energy);
  }*/

}

/*---------------------------------------------------------------------------*/

PROCESS_THREAD(udp_client_process, ev, data)
{
  static struct etimer periodic_timer;
  static unsigned count;
  static char str[32];
  clock_init();
  PROCESS_BEGIN();

  while(1) {

#ifdef MITI
    // mitigate energy attack
    if (attack_ongoing) {
    //  printf("======== sleeping for %d s ======== \n", off_seconds);
      etimer_set(&periodic_timer, off_seconds * CLOCK_SECOND);
      PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&periodic_timer));
      update_energy(false, off_seconds, true, false);
      if (energy > 400000) {
        printf("energy after sleep 1: -%lu uJoules\n", ULONG_MAX-energy);
      } else {
        printf("energy after sleep 1: %lu uJoules\n",energy);
      }
    }
#endif

    // application naturally sleeps for X seconds before doing anything
    //printf("========= application sleeping for %lu seconds =======\n", (long unsigned) SEND_INTERVAL);
    etimer_set(&periodic_timer, SEND_INTERVAL * CLOCK_SECOND);
    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&periodic_timer));

    // account for collecting sensor data
    update_energy(false, SEND_INTERVAL, true, true);
    if (energy > 400000) {
      printf("energy after application sleep and sensor: -%lu uJoules\n", ULONG_MAX-energy);
    } else {
      printf("energy after application sleep and sensor: %lu uJoules\n",energy);
    }
    if (energy > starting_energy) {
      break;
    }

#ifdef MITI
    // mitigate energy attack
    if (attack_ongoing) {
    //  printf("======== sleeping for %d s ======== \n", off_seconds);
      etimer_set(&periodic_timer, off_seconds * CLOCK_SECOND);
      PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&periodic_timer));
      update_energy(false, off_seconds, true, false);
      if (energy > 400000) {
        printf("energy after sleep 2: -%lu uJoules\n", ULONG_MAX-energy);
      } else {
        printf("energy after sleep 2: %lu uJoules\n",energy);
      }
      // energy is critically low -> MD
      // rest of the time do AI
      if (energy <= critical_energy_level) {
        off_seconds = off_seconds * 2;
      } else {
        if (off_seconds - delta > 0) {
          off_seconds = off_seconds - delta;
        }
      }
    }
    update_energy(false, 0, false, false);
    if (energy > starting_energy) {
      break;
    }
#endif

    etimer_set(&periodic_timer, SEND_INTERVAL * CLOCK_SECOND);
    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&periodic_timer));
    update_energy(false, SEND_INTERVAL, true, false);
    if (energy > 400000) {
      printf("energy after application sleep before transmit: -%lu uJoules\n", ULONG_MAX-energy);
    } else {
      printf("energy after application sleep transmit: %lu uJoules\n",energy);
    }
    if (energy > starting_energy) {
      break;
    }

    /************** Application Tasks ***************************/
    snprintf(str, sizeof(str), "hello %d", count);
    count++;
    // send packet
    update_energy(true, 0, true, false);
    if (energy > 400000) {
      printf("energy after transmit: -%lu uJoules\n", ULONG_MAX-energy);
    } else {
      printf("energy after transmit: %lu uJoules\n",energy);
    }
    /* Simulation: device ran out of energy */
    if (energy > starting_energy) {
      break;
    }
    /*************************************************************/
    
  }
  printf("out of energy!\nTime: %lucS\nPackets: %d\n", total_time, count);
  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
