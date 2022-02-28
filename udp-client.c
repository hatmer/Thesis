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

#define SEND_INTERVAL		  60// Application sends a message every X seconds

#define MITI 1


unsigned long starting_energy = 225;
static unsigned long energy = 225; // energy buffer level
static unsigned long total_time = 0;
static unsigned long last_cpu_reading, last_lpm_reading, prev_time;
static bool firstrun = true;

#ifdef MITI
static bool attack_ongoing = true;
static int off_seconds = 120;
int critical_energy_level = 80;
int delta = 10;
#endif

// energy (u joules) used per 10 milliseconds
unsigned long harvestable = 10; // amount of harvestable energy per 10 milliseconds during attack
unsigned long cpu_s_energy = 12; // amount of energy (mu amps) used by 10 cpu millisecond
unsigned long radio_s_transmit_energy = 275; // amount of energy used by a radio transmit 10 milliseconds
unsigned long radio_s_listen_energy = 275; // amount of energy used by a radio listen 10 milliseconds
unsigned long lpm_energy = 1; //.15; // LPM3

//unsigned long starting_energies[10] = {90, 135, 180, 225}; // u Joules

 
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

void update_energy(bool radio, unsigned long lpm_seconds)
{
  /*********** update energy level *************/
  energest_flush();
  
  printf("******** Updating Energy ********* \nenergy before update %lu\n", energy);

  energy = energy - ((to_centi_seconds(energest_type_time(ENERGEST_TYPE_CPU)) - last_cpu_reading - (lpm_seconds*100))*cpu_s_energy);
  
  printf("cpu: %lu ms\n", (to_centi_seconds(energest_type_time(ENERGEST_TYPE_CPU)) - last_cpu_reading - (lpm_seconds*100))*10);
  
  last_cpu_reading = to_centi_seconds(energest_type_time(ENERGEST_TYPE_CPU));
  
  if (radio) {
    // TODO listened for x milliseconds
    // transmitted for 3.3 milliseconds
    energy = energy - (radio_s_listen_energy / 3);
  }

  energy = energy - ((to_centi_seconds(energest_type_time(ENERGEST_TYPE_LPM)) - last_lpm_reading + (lpm_seconds*100))*lpm_energy);
  last_lpm_reading = to_centi_seconds(energest_type_time(ENERGEST_TYPE_LPM));
  printf("last lpm reading: %lu cs\n", last_lpm_reading);

  prev_time = total_time;
  total_time = to_centi_seconds(ENERGEST_GET_TOTAL_TIME());

  energy = energy + ((total_time - prev_time) * harvestable);
  if (energy > starting_energy) {
    if (energy < 400000000) { 
      energy = starting_energy;
    }
  }
  if (energy > 400000) {
    printf("energy after update: -%lu uJoules\n", ULONG_MAX-energy);
  } else {
    printf("energy after update: %lu uJoules\n",energy);
  }
}

/*---------------------------------------------------------------------------*/

PROCESS_THREAD(udp_client_process, ev, data)
{
  static struct etimer periodic_timer;
#ifdef MITI
  static struct etimer sleep_timer;
#endif
  static unsigned count;
  static char str[32];
  clock_init();
  PROCESS_BEGIN();

  while(1) {
#ifdef MITI
    if (firstrun) {
      energest_flush();
      last_cpu_reading = 1210;
      firstrun = false;
    }

    // mitigate energy attack
    if (attack_ongoing) {
      printf("======== sleeping for %d s ======== \n", off_seconds);
      etimer_set(&sleep_timer, off_seconds * CLOCK_SECOND);
      PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&sleep_timer));
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
    if (energy > 400000) {
      printf("energy after doze: -%lu uJoules\n", ULONG_MAX-energy);
    } else {
      printf("energy after doze: %lu uJoules\n",energy);
    }
#endif

    // application naturally sleeps for X seconds before doing anything
    printf("========= application sleeping for %lu seconds =======\n", (long unsigned) SEND_INTERVAL);
    etimer_set(&periodic_timer, SEND_INTERVAL * CLOCK_SECOND);
    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&periodic_timer));
    update_energy(false, SEND_INTERVAL + off_seconds);


    printf("======= Application Running ==========\n");
    /************** Application Tasks ***************************/
    printf("Sending request %u to server\n", count);
    snprintf(str, sizeof(str), "hello %d", count);
    count++;

    update_energy(true, 0);
    /* Simulation: device ran out of energy */
    if (energy > starting_energy) {
      break;
    }
    /*************************************************************/
    

    if (energy > 400000) {
      printf("energy after app tasks: -%lu uJoules\n", ULONG_MAX-energy);
    } else {
      printf("energy after app tasks: %lu uJoules\n",energy);
    }
    printf("total time so far: %lucS\n", total_time);
  
  }


  printf("out of energy! total time: %lucS\n", total_time);
  printf("application process done");
  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
