#ifndef MITI_H_
#define MITI_H_

// variables for using miti API
struct Miti {
  bool attack;
  double QoS;
} miti;

static int send_wrapper(int (*send), struct Miti *userVars, void **args);
//static int sense_wrapper(int (*send)(struct Miti *userVars, void **args));

#endif // MITI_H_
