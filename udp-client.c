#include "contiki.h"
#include "net/routing/routing.h"
#include "random.h"
#include "net/netstack.h"
#include "net/ipv6/simple-udp.h"
#include "miti.h"

#include "sys/energest.h"

#include "sys/log.h"

#define LOG_MODULE "App"
#define LOG_LEVEL LOG_LEVEL_INFO

#define WITH_SERVER_REPLY  1
#define UDP_CLIENT_PORT	8765
#define UDP_SERVER_PORT	5678

#define SEND_INTERVAL		  (60 * CLOCK_SECOND)

static struct simple_udp_connection udp_conn;

static inline unsigned long
to_seconds(uint64_t time)
{
  return (unsigned long)(time / ENERGEST_SECOND);
}

/*---------------------------------------------------------------------------*/
PROCESS(udp_client_process, "UDP client");
AUTOSTART_PROCESSES(&udp_client_process);
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


int on_seconds = 20; // big numbers for debugging
int off_seconds = 80;
bool ongoing_attack = true;
bool shouldSleep = false;

// starting energies: 60, 120, 180, 240, 300, 360, 420, 480, 540 mu amps 
unsigned long starting_energy = 10000;
static unsigned long energy = 10000; // energy buffer level
unsigned long harvestable = 300; // amount of harvestable energy during attack
unsigned long cpu_s_energy = 400; // amount of energy (mu amps) used by a cpu second
unsigned long radio_s_transmit_energy = 8500; // amount of energy used by a radio transmit second
unsigned long radio_s_listen_energy = 8500; // amount of energy used by a radio listen second
unsigned long lpm_energy = 1.5; // LPM3
static unsigned long last_cpu_reading, last_radio_transmit_reading, last_radio_listen_reading, last_lpm_reading;
//uint64_t


static void doze(void *ptr){
  LOG_INFO("sleeping\n");
  NETSTACK_RADIO.off();
  printf("sleep for %d\n", off_seconds);
  shouldSleep = true; // triggers cpu sleep timer
}

static void wake(){
  LOG_INFO("waking\n");
  NETSTACK_RADIO.on();
  shouldSleep = false;
}



/*---------------------------------------------------------------------------*/
PROCESS_THREAD(udp_client_process, ev, data)
{
  static struct etimer periodic_timer;
  static struct etimer sleep_timer;
  static struct ctimer ct;
  static unsigned count;
  static char str[32];
  uip_ipaddr_t dest_ipaddr;

  PROCESS_BEGIN();

  /* Initialize UDP connection */
  simple_udp_register(&udp_conn, UDP_CLIENT_PORT, NULL,
                      UDP_SERVER_PORT, udp_rx_callback);

  etimer_set(&periodic_timer, random_rand() % SEND_INTERVAL);

  // main loop
  while(1) {
    /*********** Conserve Energy ************/

    // should conserve energy -> set doze timer
    if (shouldSleep) {
      etimer_set(&sleep_timer, off_seconds*CLOCK_SECOND);
    }
    // low power state until timer expires
    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&sleep_timer));
    // wake up from sleep
    if (shouldSleep) {
        wake();
    }

    // be awake for 5 seconds, but
    // after 5 seconds, turn off peripherals and suspend process (doze)
    printf("waking for %d\n", on_seconds);
    ctimer_set(&ct, CLOCK_SECOND * on_seconds, doze, NULL);

    /******************************************/

    // cannot use two etimers...
    //PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&periodic_timer));
 
    // DEBUG: verify reachability
    if(NETSTACK_ROUTING.node_is_reachable()){
      printf("is reachable");
    } else {
      printf("not reachable");
    }

    // application tasks
    if(NETSTACK_ROUTING.node_is_reachable() && NETSTACK_ROUTING.get_root_ipaddr(&dest_ipaddr)) {
      // send to base station
      LOG_INFO("Sending request %u to ", count);
      LOG_INFO_6ADDR(&dest_ipaddr);
      LOG_INFO_("\n");
      snprintf(str, sizeof(str), "hello %d", count);
      simple_udp_sendto(&udp_conn, str, strlen(str), &dest_ipaddr);
      count++;

      } else {
        LOG_INFO("Not reachable yet\n");
    }

    etimer_set(&periodic_timer, SEND_INTERVAL);

    // update energy level
    energest_flush();

   // unsigned long cpu_on_energy = ((to_seconds(energest_type_time(ENERGEST_TYPE_CPU)) - last_cpu_reading)*cpu_s_energy);
   // printf("cpu on energy: %lu\n", cpu_on_energy);
    energy = energy - ((to_seconds(energest_type_time(ENERGEST_TYPE_CPU)) - last_cpu_reading)*cpu_s_energy);
    last_cpu_reading = to_seconds(energest_type_time(ENERGEST_TYPE_CPU));

//    unsigned long radio_transmit_energy = ((to_seconds(energest_type_time(ENERGEST_TYPE_TRANSMIT)) - last_radio_transmit_reading)*radio_s_transmit_energy);
  //  printf("radio transmit energy: %lu\n", radio_transmit_energy);
    energy = energy - ((to_seconds(energest_type_time(ENERGEST_TYPE_TRANSMIT)) - last_radio_transmit_reading)*radio_s_transmit_energy);
    last_radio_transmit_reading = to_seconds(energest_type_time(ENERGEST_TYPE_TRANSMIT));

    energy = energy - ((to_seconds(energest_type_time(ENERGEST_TYPE_LISTEN)) - last_radio_listen_reading)*radio_s_listen_energy);
    last_radio_listen_reading = to_seconds(energest_type_time(ENERGEST_TYPE_LISTEN));

    energy = energy - ((to_seconds(energest_type_time(ENERGEST_TYPE_LPM)) - last_lpm_reading)*lpm_energy);
    last_lpm_reading = to_seconds(energest_type_time(ENERGEST_TYPE_LPM));
    // ToDo deep sleep energy use etc.

    if (energy > starting_energy) {
      printf("out of energy!\n");
      break;
    }

    printf("energy level: %lu\n", energy);

    // DEBUG: print energy usage
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


  }


  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
