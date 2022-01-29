#ifndef MITI_H_
#define MITI_H_

#include "contiki.h"
#include "net/routing/routing.h"
#include "net/netstack.h"
#include "net/ipv6/simple-udp.h"
#include "sensors.h"

typedef struct simple_udp_connection simple_udp_connection;
typedef struct Miti Miti;

struct Miti {
  int attack;
  int attackTimeIdx;
  double QoS;
  struct sensors_sensor pir_sensor;
} miti;

int send_wrapper(int (*send)(simple_udp_connection*, const void*, uint16_t, const uip_ipaddr_t*),  simple_udp_connection* udp_conn, const void* str, uint16_t length, const uip_ipaddr_t *dest_ipaddr, Miti* userVars);

/*int sense_wrapper(int (*sense)(simple_udp_connection*), Miti* userVars);*/

int get_time();

#endif // MITI_H_
