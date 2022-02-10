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

#define SEND_INTERVAL		  (15 * CLOCK_SECOND) // Application sends a message every X seconds
#define attack_start      (60 * CLOCK_SECOND) // Attack starts after X seconds
static struct simple_udp_connection udp_conn;
static process_event_t attack_event;
static process_event_t wake_event;
static process_event_t continue_event;

unsigned long starting_energy = 1000000;
static unsigned long energy = 1000000; // energy buffer level
static unsigned long total_time = 0;
static bool attack_ongoing = false;
static unsigned long last_cpu_reading, last_radio_transmit_reading, last_radio_listen_reading, last_lpm_reading;

int attack_duration = 240;
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
PROCESS(attack_mitigation_process, "Attack Mitigation Process");
PROCESS(attack_detection_process, "Simulated Attack Detection Process");
//PROCESS(critical_energy_detection_process, "Critical Energy Level Detection Process");
AUTOSTART_PROCESSES(&udp_client_process, &attack_mitigation_process, &attack_detection_process);//, &critical_energy_detection_process);
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

PROCESS_THREAD(attack_mitigation_process, ev, data)
{
  static struct etimer sleep_timer;

  PROCESS_BEGIN();
  wake_event = process_alloc_event();

  while (1) {
      // an attack is ongoing -> sleep for off_seconds seconds
    printf("attack mitigation process waiting for signal from detection process\n");
    PROCESS_WAIT_EVENT_UNTIL(ev == attack_event || ev == continue_event);

    /* Simulate device running out of energy */
    if (energy > starting_energy) {
      printf("out of energy!\n");
      break;
    }
    /*****************************************/

    printf("sleeping\n");
    NETSTACK_RADIO.off();
    printf("sleep for %d\n", off_seconds);
    etimer_set(&sleep_timer, off_seconds * CLOCK_SECOND);
    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&sleep_timer));
    NETSTACK_RADIO.on();
    printf("waking\n");

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

    update_energy();
    /* Simulation: device ran out of energy */
    if (energy > starting_energy) {
      printf("out of energy! total time: %lus\n", total_time);
      break;
    }
    /*****************************************/
    process_post(&udp_client_process, wake_event, NULL);
    attack_ongoing = false;
  }

  PROCESS_END();
}
/*
PROCESS_THREAD(critical_energy_detection_process, ev, data)
{
  static struct etimer check_timer;
  PROCESS_BEGIN();
  while (1) {
    if (energy <= critical_energy_level) {
      energy_is_critical = true;
    } else {
      energy_is_critical = false;
    }
    etimer_set(&check_timer, 20);
    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&check_timer));
  }
  PROCESS_END();
}
*/
PROCESS_THREAD(attack_detection_process, ev, data)
{
  static struct etimer attack_timer;

  PROCESS_BEGIN();
  attack_event = process_alloc_event();
  // simulated attack detection
  etimer_set(&attack_timer, attack_start);
  PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&attack_timer));
  printf("sending attack event to mitigation process\n");
  attack_ongoing = true;
  process_post(&attack_mitigation_process, attack_event, NULL);
  //etimer_set(&attack_timer, attack_duration*CLOCK_SECOND);
  //PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&attack_timer));
  //etimer_reset(&attack_timer);
  //attack_ongoing = false;
  //printf("attack detection system reports attack over\n");

  // attack over
  PROCESS_END();
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

  LOG_INFO("Received response '%.*s' from ", datalen, (char *) data);
  LOG_INFO_6ADDR(sender_addr);
#if LLSEC802154_CONF_ENABLED
  LOG_INFO_(" LLSEC LV:%d", uipbuf_get_attr(UIPBUF_ATTR_LLSEC_LEVEL));
#endif
  LOG_INFO_("\n");

}
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(udp_client_process, ev, data)
{
  static struct etimer periodic_timer;
  static unsigned count;
  static char str[32];
  uip_ipaddr_t dest_ipaddr;

  PROCESS_BEGIN();
  continue_event = process_alloc_event();

  /* Initialize UDP connection */
  simple_udp_register(&udp_conn, UDP_CLIENT_PORT, NULL,
                      UDP_SERVER_PORT, udp_rx_callback);

  //etimer_set(&periodic_timer, random_rand() % SEND_INTERVAL);
  while(1) {
    LOG_INFO("main process looping... attack ongoing?\n");
    if (attack_ongoing) { printf("yes"); }


    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&periodic_timer));
    printf("periodic timer expired. attack ongoing?");
    if (attack_ongoing) { printf("yes"); }

    if (attack_ongoing) {
      LOG_INFO("waiting for go-ahead\n");
      PROCESS_WAIT_EVENT_UNTIL(ev == wake_event);
      LOG_INFO("main process received go-ahead\n");
    }

    /* Simulation: device ran out of energy */
    if (energy > starting_energy) {
      printf("out of energy! total time: %lus\n", total_time);
      break;
    }
    /*****************************************/
    NETSTACK_RADIO.on();
    if(NETSTACK_ROUTING.node_is_reachable() && NETSTACK_ROUTING.get_root_ipaddr(&dest_ipaddr)) {
      /* Send to DAG root */
      LOG_INFO("Sending request %u to ", count);
      LOG_INFO_6ADDR(&dest_ipaddr);
      LOG_INFO_("\n");
      snprintf(str, sizeof(str), "hello %d", count);
      simple_udp_sendto(&udp_conn, str, strlen(str), &dest_ipaddr);
      count++;
    } else {
      LOG_INFO("Not reachable yet\n");
    }

    /* Add some jitter */
    etimer_set(&periodic_timer, SEND_INTERVAL
      /*- CLOCK_SECOND + (random_rand() % (2 * CLOCK_SECOND))*/);

    update_energy();
    /* Simulation: device ran out of energy */
    if (energy > starting_energy) {
      LOG_INFO("out of energy! total time: %lus\n", total_time);
      break;
    }
    /*****************************************/
    LOG_INFO("energy level: %lu\n", energy);

    // continue mitigating attack 
    if (attack_ongoing) {
      process_post(&attack_mitigation_process, continue_event, NULL);
    }

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
  printf("application process done");
  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
