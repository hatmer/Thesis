#include "contiki.h"
#include "net/netstack.h"
#include "net/routing/routing.h"
#include "random.h"
#include "net/netstack.h"
#include "net/ipv6/simple-udp.h"

#include "sys/log.h"
#define LOG_MODULE "App"
#define LOG_LEVEL LOG_LEVEL_INFO

#include <stdio.h> /* For printf() */
/*---------------------------------------------------------------------------*/
PROCESS(demo_app_process, "Demo app process");
AUTOSTART_PROCESSES(&demo_app_process);
/*---------------------------------------------------------------------------*/

/* network peripheral */
static void
send(char* data)
{
  radio_value_t chan;

  if(NETSTACK_RADIO.get_value(RADIO_PARAM_CHANNEL, &chan) == RADIO_RESULT_OK) {
    uart0_writeb(chan & 0xFF);
    return;
  }
}


/* sensor peripheral */


PROCESS_THREAD(demo_app_process, ev, data)
{
  static struct etimer timer;

  PROCESS_BEGIN();

  /* Setup a periodic timer that expires after 10 seconds. */
  etimer_set(&timer, CLOCK_SECOND * 10);

  while(1) {
    send("Hello, world\n");

    /* Wait for the periodic timer to expire and then restart the timer. */
    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&timer));
    etimer_reset(&timer);
  }

  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
