#include "contiki.h"
#include "net/routing/routing.h"
#include "random.h"
#include "net/netstack.h"
#include "net/ipv6/simple-udp.h"
#include "miti.h"
#include "sensors.h"
#include "pir-sensor.h"

#include "sys/log.h"
#include "energest.h"

#define LOG_MODULE "App"
#define LOG_LEVEL LOG_LEVEL_INFO

#define WITH_SERVER_REPLY  1
#define UDP_CLIENT_PORT	8765
#define UDP_SERVER_PORT	5678

#define SEND_INTERVAL		  (60 * CLOCK_SECOND)

static struct simple_udp_connection udp_conn;
const struct sensors_sensor button_sensor;

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

static unsigned long
to_seconds(uint64_t time)
{
  return (unsigned long)(time / ENERGEST_SECOND);
}

/*---------------------------------------------------------------------------*/
PROCESS_THREAD(udp_client_process, ev, data)
{
  static struct etimer periodic_timer;
  static struct etimer sleep_timer;
  static unsigned count;
  static char str[32];
  uip_ipaddr_t dest_ipaddr;
  struct Miti miti_vars;
  miti_vars.attack = false;
  miti_vars.QoS = .2;
  miti_vars.pir_sensor = pir_sensor;


  PROCESS_BEGIN();
  
  /* Initialize UDP connection */
  simple_udp_register(&udp_conn, UDP_CLIENT_PORT, NULL,
                      UDP_SERVER_PORT, udp_rx_callback);


  etimer_set(&periodic_timer, SEND_INTERVAL);

  // Main loop
  while(1) {
      PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&periodic_timer)); 
      LOG_INFO("client sending\n");
      
      /* Send data to server */
      LOG_INFO("Sending request %u to ", count);
      LOG_INFO_6ADDR(&dest_ipaddr);
      LOG_INFO_("\n");
      
      /* Attack mitigation */
      LOG_INFO("checking if should be asleep\n");
      int t = get_time();
      printf("time is %d\n", t);
      if ( /*userVars->attack && */ ((t % 100) / 10) > (/*state.wakePercent*/ .2 * 10))
      {
        int sleepTime = 10 - ((t % 100) / 10);
        printf("sleeping until: %d\n", sleepTime);
        LOG_INFO("sleeping\n");
        // NETSTACK_RADIO.off();
        SENSORS_DEACTIVATE(pir_sensor);

        // sleep until next wake period
        printf("sleep for %d\n", sleepTime);
        etimer_set(&sleep_timer, sleepTime*CLOCK_SECOND); // *1000 for real-time
        PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&sleep_timer));

        LOG_INFO("waking up\n");
        SENSORS_ACTIVATE(pir_sensor);
        // NETSTACK_RADIO.on();
      }


      simple_udp_sendto(&udp_conn, str, strlen(str), &dest_ipaddr);
      
      send_wrapper(simple_udp_sendto, &udp_conn, str, strlen(str), &dest_ipaddr, &miti_vars);
      count++;
      LOG_INFO("send complete. send count: %d\n", count);

      etimer_set(&periodic_timer, SEND_INTERVAL);

      energest_flush();
      printf("\nEnergest:\n");
      printf(" CPU     %4lus LPM     %4lus DEEP LPM %lus   Total time %lus\n",
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
