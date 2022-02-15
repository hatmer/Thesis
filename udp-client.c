#include "contiki.h"
#include "net/routing/routing.h"
#include "random.h"
#include "net/netstack.h"
#include "net/ipv6/simple-udp.h"

#include "sys/energest.h"

#include "sys/log.h"
#define LOG_MODULE "App"
#define LOG_LEVEL LOG_LEVEL_INFO

#define WITH_SERVER_REPLY  1
#define UDP_CLIENT_PORT	8765
#define UDP_SERVER_PORT	5678

#define SEND_INTERVAL		  (6 * CLOCK_SECOND) // Application sends a message every X seconds
#define attack_start      (6 * CLOCK_SECOND) // Attack starts after X seconds
#define attack_duration   (24 * CLOCK_SECOND)

#define MITI 1

static struct simple_udp_connection udp_conn;

unsigned long starting_energy = 225;
static unsigned long energy = 225; // energy buffer level
static unsigned long tmp_energy = 0;
static unsigned long total_time = 0;
static unsigned long tmp_time = 0;
static unsigned long rmv_time = 0;
static unsigned long last_cpu_reading, last_radio_transmit_reading, last_radio_listen_reading, last_lpm_reading;

#ifdef MITI
static bool attack_ongoing = true;
static int off_seconds = 12;
int critical_energy_level = 300;
int delta = 10;
#endif

static bool firstrun = true;
// energy (u joules) used per 10 milliseconds
unsigned long harvestable = 3; // amount of harvestable energy per 10 milliseconds during attack
unsigned long cpu_s_energy = 4; // amount of energy (mu amps) used by 10 cpu millisecond
unsigned long radio_s_transmit_energy = 85; // (about 34 u joules per transmission) amount of energy used by a radio transmit millisecond
unsigned long radio_s_listen_energy = 85; // amount of energy used by a radio listen 10 millisecond
unsigned long lpm_energy = 0; //.15; // LPM3

//unsigned long starting_energies[10] = {22, 45, 67, 90, 112, 135, 157, 180, 202, 225}; // u Joules

 
static inline unsigned long to_milli_seconds(uint64_t time)
{
  //printf("returning this many time units: %lu\n", (unsigned long)(time / (ENERGEST_SECOND / 100)));
  return (unsigned long)(time / (ENERGEST_SECOND / 100));
}

/*---------------------------------------------------------------------------*/
PROCESS(udp_client_process, "Demo Application (UDP client)");
AUTOSTART_PROCESSES(&udp_client_process);
/*---------------------------------------------------------------------------*/

void update_energy()
{
  /*********** update energy level *************/
  energest_flush();
  printf("energy before update %lu\n", energy);
  energy = energy - ((to_milli_seconds(energest_type_time(ENERGEST_TYPE_CPU)) - last_cpu_reading)*cpu_s_energy);
  last_cpu_reading = to_milli_seconds(energest_type_time(ENERGEST_TYPE_CPU));
  printf("last cpu reading: %lu\n", last_cpu_reading);
  printf("energy is now %lu\n", energy);
  energy = energy - ((to_milli_seconds(energest_type_time(ENERGEST_TYPE_TRANSMIT)) - last_radio_transmit_reading)*radio_s_transmit_energy);
  last_radio_transmit_reading = to_milli_seconds(energest_type_time(ENERGEST_TYPE_TRANSMIT));
  printf("last radio transmit reading: %lu\n", last_radio_transmit_reading);
  energy = energy - ((to_milli_seconds(energest_type_time(ENERGEST_TYPE_LISTEN)) - last_radio_listen_reading)*radio_s_listen_energy);
  last_radio_listen_reading = to_milli_seconds(energest_type_time(ENERGEST_TYPE_LISTEN));
  printf("last radio listen reading: %lu\n", last_radio_listen_reading);
  energy = energy - ((to_milli_seconds(energest_type_time(ENERGEST_TYPE_LPM)) - last_lpm_reading)*lpm_energy);
  last_lpm_reading = to_milli_seconds(energest_type_time(ENERGEST_TYPE_LPM));

  total_time = to_milli_seconds(ENERGEST_GET_TOTAL_TIME());

  printf("energy after update %lu\n", energy);
}


/*---------------------------------------------------------------------------*/
static void
udp_rx_callback(struct simple_udp_connection *c,
         const uip_ipaddr_t *sender_addr,
         uint16_t sender_port,
         const uip_ipaddr_t *receiver_addr,
         uint16_t receiver_port,
         const uint8_t *data,
         uint16_t datalen)
{

  printf("Received response '%.*s' from client\n", datalen, (char *) data);
}
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(udp_client_process, ev, data)
{
  static struct etimer periodic_timer;
  static struct etimer sleep_timer;
  static unsigned count;
  static char str[32];
  uip_ipaddr_t dest_ipaddr;

  PROCESS_BEGIN();

  /* Initialize UDP connection */
  simple_udp_register(&udp_conn, UDP_CLIENT_PORT, NULL,
                      UDP_SERVER_PORT, udp_rx_callback);

  while(1) {
#ifdef MITI
    if (firstrun) {
      energy = energy + (4*85); // 40 milliseconds of radio listen...?
      firstrun = false;
    }
    //update_energy();
    // mitigate energy attack
    if (attack_ongoing) {
      //printf("M: sleeping\n");
      NETSTACK_RADIO.off();
      printf("M: sleep for %d\n", off_seconds);
      etimer_set(&sleep_timer, off_seconds * CLOCK_SECOND);
      PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&sleep_timer));
      //NETSTACK_RADIO.on();
      printf("M: waking\n");
      // energy is critically low -> MD
      // rest of the time do AI
      if (energy <= critical_energy_level) {
        off_seconds = off_seconds * 2;
      } else {
        if (off_seconds - delta >= 0) {
          off_seconds = off_seconds - delta;
        }
      }
    }
    update_energy();
    tmp_energy = energy;
    tmp_time = total_time;
    printf("energy is now %lu\n", energy);
    printf("tmp energy is now %lu\n", tmp_energy);
#endif
    // application naturally sleeps for X seconds before doing anything
    etimer_set(&periodic_timer, SEND_INTERVAL);
    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&periodic_timer));
#ifdef MITI
    NETSTACK_RADIO.on();
    // wait for netstack to turn back on...
    printf("simulation waiting for netstack to turn back on\n");
    etimer_set(&periodic_timer, SEND_INTERVAL);
    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&periodic_timer));
    update_energy();
    energy = tmp_energy;
    rmv_time = rmv_time + (tmp_time - total_time);

    /* Simulation: device ran out of energy? */
    if (energy > starting_energy) {
      break;
    }
#endif

    /************** Application Tasks ***************************/
    if(NETSTACK_ROUTING.node_is_reachable() && NETSTACK_ROUTING.get_root_ipaddr(&dest_ipaddr)) {
      printf("C: Sending request %u to server\n", count);
      snprintf(str, sizeof(str), "hello %d", count);
      simple_udp_sendto(&udp_conn, str, strlen(str), &dest_ipaddr);
      count++;
    } else {
      printf("C: Not reachable yet\n");
    }

    update_energy();
    /* Simulation: device ran out of energy */
    if (energy > starting_energy) {
      break;
    }
    /*************************************************************/
    printf("C: energy level: %lu\n", energy);

  }
  printf("C: out of energy! total time: %lus\n", (total_time - rmv_time) / 100);
  printf("C: application process done");
  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
