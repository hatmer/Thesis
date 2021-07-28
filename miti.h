#ifndef MITI_H_
#define MITI_H_

// variables for using miti API
struct Miti {
  bool attack;
  double QoS;
} miti;

int send_wrapper(int (*send)(struct simple_udp_connection*, const void*, uint16_t, const uip_ipaddr_t*),  struct simple_udp_connection* udp_conn, const void* str, int length, const uip_ipaddr_t *dest_ipaddr, struct Miti* userVars);
//static int sense_wrapper(int (*send)(struct Miti *userVars, void **args));

#endif // MITI_H_
