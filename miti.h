#ifndef MITI_H_
#define MITI_H_

#include "contiki.h"
#include "net/routing/routing.h"
#include "net/netstack.h"
#include "net/ipv6/simple-udp.h"

typedef struct simple_udp_connection simple_udp_connection;

// variables for using miti API
typedef struct Miti Miti;
struct Miti {
  int attack;
  double QoS;
} miti;

int send_wrapper(int (*send)(simple_udp_connection**, const void*, uint16_t, const uip_ipaddr_t*),  simple_udp_connection** udp_conn, const void* str, uint16_t length, const uip_ipaddr_t *dest_ipaddr, Miti* userVars);
//static int sense_wrapper(int (*send)(struct Miti *userVars, void **args));

#endif // MITI_H_
