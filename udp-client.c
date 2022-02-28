#include "contiki.h"
//#include "net/routing/routing.h"
#include "random.h"
//#include "net/netstack.h"
//#include "net/ipv6/simple-udp.h"
#include <limits.h>
#include "sys/energest.h"
//#include "net/ipv6/uip-udp-packet.h"
#include "sys/log.h"
#define LOG_MODULE "App"
#define LOG_LEVEL LOG_LEVEL_INFO

#define WITH_SERVER_REPLY  1
#define UDP_CLIENT_PORT	8765
#define UDP_SERVER_PORT	5678

#define SEND_INTERVAL		  (60 * CLOCK_SECOND) // Application sends a message every X seconds
#define attack_start      (65 * CLOCK_SECOND) // Attack starts after X seconds
#define attack_duration   (240 * CLOCK_SECOND)

#define MITI 1

//static struct simple_udp_connection udp_conn;
//static struct uip_udp_conn* conn;



unsigned long starting_energy = 225;
static unsigned long energy = 225; // energy buffer level
static unsigned long total_time = 0;
static unsigned long rmv_time = 0;
static unsigned long last_cpu_reading, /*last_radio_transmit_reading, last_radio_listen_reading,*/ last_lpm_reading, prev_time;
static unsigned long tmp_time = 0;
static unsigned long tmp_energy = 0;

#ifdef MITI
static bool attack_ongoing = true;
static int off_seconds = 12;
int critical_energy_level = 80;
int delta = 10;
#endif

// energy (u joules) used per 10 milliseconds
unsigned long harvestable = 1; // amount of harvestable energy per 10 milliseconds during attack
unsigned long cpu_s_energy = 12; // amount of energy (mu amps) used by 10 cpu millisecond
unsigned long radio_s_transmit_energy = 275; // (about 34 u joules per transmission) amount of energy used by a radio transmit millisecond
unsigned long radio_s_listen_energy = 275; // amount of energy used by a radio listen 10 millisecond
unsigned long lpm_energy = 0; //.15; // LPM3

//unsigned long starting_energies[10] = {22, 45, 67, 90, 112, 135, 157, 180, 202, 225}; // u Joules

 
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

void update_energy(bool show)
{
  /*********** update energy level *************/
  energest_flush();
  if (show)
    printf("******** Updating Energy ********* \nenergy before update %lu\n", energy);

  energy = energy - ((to_centi_seconds(energest_type_time(ENERGEST_TYPE_CPU)) - last_cpu_reading)*cpu_s_energy);
  if (show)
    printf("cpu: %lu ms\n", (to_centi_seconds(energest_type_time(ENERGEST_TYPE_CPU)) - last_cpu_reading)*10);
  last_cpu_reading = to_centi_seconds(energest_type_time(ENERGEST_TYPE_CPU));
  /*
  energy = energy - ((to_centi_seconds(energest_type_time(ENERGEST_TYPE_TRANSMIT)) - last_radio_transmit_reading)*radio_s_transmit_energy);
  if (show)
    printf("radio transmit: %lu ms\n", (to_centi_seconds(energest_type_time(ENERGEST_TYPE_TRANSMIT)) - last_radio_transmit_reading) * 10);
  last_radio_transmit_reading = to_centi_seconds(energest_type_time(ENERGEST_TYPE_TRANSMIT));
*/
  /* Ignore Listen time for now since it is too high */
  //energy = energy - ((to_centi_seconds(energest_type_time(ENERGEST_TYPE_LISTEN)) - last_radio_listen_reading)*radio_s_listen_energy);
  //if (show)
  //  printf("radio listen: %lu ms\n", (to_centi_seconds(energest_type_time(ENERGEST_TYPE_LISTEN)) - last_radio_listen_reading)*10);
  //last_radio_listen_reading = to_centi_seconds(energest_type_time(ENERGEST_TYPE_LISTEN));
  

  energy = energy - ((to_centi_seconds(energest_type_time(ENERGEST_TYPE_LPM)) - last_lpm_reading)*lpm_energy);
  last_lpm_reading = to_centi_seconds(energest_type_time(ENERGEST_TYPE_LPM));

  prev_time = total_time;
  total_time = to_centi_seconds(ENERGEST_GET_TOTAL_TIME());

  // 
  energy = energy + ((total_time - prev_time) * harvestable);
  if (energy > starting_energy) {
    if (energy < 400000000) { 
      energy = starting_energy;
    }
  }
  if (show) {
  if (energy > 400000) {
    printf("energy after update: -%lu uJoules\n", ULONG_MAX-energy);
  } else {
    printf("energy after update: %lu uJoules\n",energy);
  }
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
  uip_ipaddr_t dest_ipaddr;

  PROCESS_BEGIN();

  //conn = udp_new(NULL, 0, NULL); 
  //udp_bind(conn, UDP_CLIENT_PORT);

  while(1) {
#ifdef MITI
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
    //update_energy(true);
    //tmp_energy = energy;
    if (energy > 400000) {
    printf("energy after doze: -%lu uJoules\n", ULONG_MAX-energy);
  } else {
    printf("energy after doze: %lu uJoules\n",energy);
  }
#endif
    //tmp_time = total_time;
    // application naturally sleeps for X seconds before doing anything
    printf("========= application sleeping for %lu seconds =======\n", SEND_INTERVAL);
    etimer_set(&periodic_timer, SEND_INTERVAL);
    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&periodic_timer));
    update_energy(true);

    //NETSTACK_RADIO.on();
    // Simulation: wait for netstack to turn back on. Otherwise packet sometimes isn't sent. This wait is "as if it didn't happen" and is only part of the simulation.
   /* printf("==== simulation waiting for netstack to turn back on =======\n");
    etimer_set(&periodic_timer, SEND_INTERVAL);
    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&periodic_timer));
    update_energy(false);
    energy = tmp_energy;
    rmv_time = rmv_time + (total_time-tmp_time);
*/
    /* Simulation: device ran out of energy? */
    //if (energy > starting_energy) {
   //   break;
    //}

    printf("======= Application Running ==========\n");
    /************** Application Tasks ***************************/
    if(NETSTACK_ROUTING.node_is_reachable() && NETSTACK_ROUTING.get_root_ipaddr(&dest_ipaddr)) {
      printf("Sending request %u to server\n", count);
      snprintf(str, sizeof(str), "hello %d", count);
      //unsigned long time_before = to_milli_seconds(ENERGEST_GET_TOTAL_TIME());
      //uip_udp_packet_sendto(conn, str, strlen(str), &dest_ipaddr, UIP_HTONS(UDP_SERVER_PORT));
      //simple_udp_sendto(&udp_conn, str, strlen(str), &dest_ipaddr);
      //unsigned long time_after = to_milli_seconds(ENERGEST_GET_TOTAL_TIME());
      //printf("clock seconds: %lu\n", CLOCK_SECOND);
      //printf("number of 10000th seconds transmitting: %lu\n", ((time_after - time_before) / 8) * 625);
      //printf("before: %lu, after: %lu", time_before, time_after);

      //NETSTACK_RADIO.off();
      count++;
    } else {
      printf("Not reachable yet\n");
    }

    update_energy(true);
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
  printf("out of energy! total time: %lucS\n", total_time); // - rmv_time));
  printf("application process done");
  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
