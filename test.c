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

#define SEND_INTERVAL		  (60 * CLOCK_SECOND) // Application sends a message every X seconds
#define attack_start      (60 * CLOCK_SECOND) // Attack starts after X seconds
#define attack_duration   (240 * CLOCK_SECOND)

static struct simple_udp_connection udp_conn;

unsigned long starting_energy = 10000000;
static unsigned long energy = 10000000; // energy buffer level
static unsigned long total_time = 0;
static bool attack_ongoing = true;
static unsigned long last_cpu_reading, last_radio_transmit_reading, last_radio_listen_reading, last_lpm_reading;

static int off_seconds = 120;
//int critical_energy_level = 300;
//int delta = 10;

unsigned long harvestable = 300; // amount of harvestable energy during attack
unsigned long cpu_s_energy = 400; // amount of energy (mu amps) used by a cpu second
unsigned long radio_s_transmit_energy = 8500; // amount of energy used by a radio transmit second
unsigned long radio_s_listen_energy = 8500; // amount of energy used by a radio listen second
unsigned long lpm_energy = 1.5; // LPM3


static inline unsigned long to_seconds(uint64_t time)
{
  return (unsigned long)(time / ENERGEST_SECOND);
}

/*---------------------------------------------------------------------------*/
PROCESS(udp_client_process, "Demo Application (UDP client)");
//PROCESS(attack_mitigation_process, "Attack Mitigation Process");
//PROCESS(attack_detection_process, "Simulated Attack Detection Process");
AUTOSTART_PROCESSES(&udp_client_process);//, &attack_mitigation_process);//, &attack_detection_process);
/*---------------------------------------------------------------------------*/

void update_energy()
{
  /*********** update energy level *************/
  energest_flush();

  energy = energy - ((to_seconds(energest_type_time(ENERGEST_TYPE_CPU)) - last_cpu_reading)*cpu_s_energy);
  last_cpu_reading = to_seconds(energest_type_time(ENERGEST_TYPE_CPU));

  energy = energy - ((to_seconds(energest_type_time(ENERGEST_TYPE_TRANSMIT)) - last_radio_transmit_reading)*radio_s_transmit_energy);
  last_radio_transmit_reading = to_seconds(energest_type_time(ENERGEST_TYPE_TRANSMIT));

  energy = energy - ((to_seconds(energest_type_time(ENERGEST_TYPE_LISTEN)) - last_radio_listen_reading)*radio_s_listen_energy);
  last_radio_listen_reading = to_seconds(energest_type_time(ENERGEST_TYPE_LISTEN));

  energy = energy - ((to_seconds(energest_type_time(ENERGEST_TYPE_LPM)) - last_lpm_reading)*lpm_energy);
  last_lpm_reading = to_seconds(energest_type_time(ENERGEST_TYPE_LPM));

  total_time = total_time + to_seconds(ENERGEST_GET_TOTAL_TIME());

  /*********************************************/
}

   //   etimer_set(&sleep_timer, 10 * CLOCK_SECOND);
   //   PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&sleep_timer));
      
      // energy is critically low -> MD
      // rest of the time do AI
      /*
      if (energy <= critical_energy_level) {
        off_seconds = off_seconds * 2;
      } else {
        if (off_seconds - delta >= 0) {
          off_seconds = off_seconds - delta;
        }
      }*/


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
  //mitigate = process_alloc_event();

  /* Initialize UDP connection */
  simple_udp_register(&udp_conn, UDP_CLIENT_PORT, NULL,
                      UDP_SERVER_PORT, udp_rx_callback);

  while(1) {
    printf("C: main process looping, attack ongoing?\n");
    if (attack_ongoing) { printf("yes\n"); }

    // mitigate energy attack
    if (attack_ongoing) {
      printf("M: sleeping\n");
      NETSTACK_RADIO.off();
      printf("M: sleep for %d\n", off_seconds);
      etimer_set(&sleep_timer, off_seconds * CLOCK_SECOND);
      PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&sleep_timer));
      NETSTACK_RADIO.on();
      printf("M: waking\n");
    }
    etimer_set(&periodic_timer, SEND_INTERVAL);
    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&periodic_timer));

    /* Simulation: device ran out of energy */
    if (energy > starting_energy) {
      printf("C: out of energy! total time: %lus\n", total_time);
      break;
    }
    /*****************************************/
    NETSTACK_RADIO.on();
    if(NETSTACK_ROUTING.node_is_reachable() && NETSTACK_ROUTING.get_root_ipaddr(&dest_ipaddr)) {
      /* Send to DAG root */
      printf("C: Sending request %u to server\n", count);
      //printf_6ADDR(&dest_ipaddr);
      //printf_("\n");
      snprintf(str, sizeof(str), "hello %d", count);
      simple_udp_sendto(&udp_conn, str, strlen(str), &dest_ipaddr);
      count++;
    } else {
      printf("C: Not reachable yet\n");
    }

    update_energy();
    /* Simulation: device ran out of energy */
    if (energy > starting_energy) {
      printf("C: out of energy! total time: %lus\n", total_time);
      break;
    }
    /*****************************************/
    printf("C: energy level: %lu\n", energy);

    // continue mitigating attack 
    //if (attack_ongoing) {
    //  process_post(&attack_mitigation_process, continue_event, NULL);
    //}

/*
    energest_flush();

    printf("\nEnergest:\n");
    printf(" CPU          %4lus LPM      %4lus DEEP LPM %4lus  Total time %lus\n",
           to_seconds(energest_type_time(ENERGEST_TYPE_CPU)),
           to_seconds(energest_type_time(ENERGEST_TYPE_LPM)),
           to_seconds(energest_type_time(ENERGEST_TYPE_DEEP_LPM)),
           to_seconds(ENERGEST_GET_TOTAL_TIME()));
    printf(" Radio LISTEN %4lus TRANSMIT %4lus OFF      %4lus\n",
           to_seconds(energest_type_time(ENERGEST_TYPE_LISTEN)),
           to_seconds(energest_type_time(ENERGEST_TYPE_TRANSMIT)),
           to_seconds(ENERGEST_GET_TOTAL_TIME()
                      - energest_type_time(ENERGEST_TYPE_TRANSMIT)
                      - energest_type_time(ENERGEST_TYPE_LISTEN)));

*/
  }
  printf("C: application process done");
  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
