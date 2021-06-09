/**
 * Attack Mitigation system for energy harvesting intermittent devices
 * 
 * Author: Hannah Atmer <email>
 *
 */

#include <unistd.h>
#include <sys/time.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

/* Size of the matrices to multiply */
#define SIZE 1000

struct 

/**
 * Proceedures
 */

/**
 * Get the time in seconds since some arbitrary point. Used for high
 * precision timing measurements.
 */
static double
get_time()
{
        struct timeval tv;

        if (gettimeofday(&tv, NULL)) {
                fprintf(stderr, "gettimeofday failed. Aborting.\n");
                abort();
        }
        return tv.tv_sec + tv.tv_usec * 1E-6;
}

/*
 * Get the energy percentage from the energy buffer
 *
 */
static double
get_energy()
{
  return 100;
}

/**
 * Wrap a network device
 */
static void
send_wrapper()
{
        int i, j;

        for (i = 0; i < SIZE; i++) {
                for (j = 0; j < SIZE; j++) {
                        mat_a[i][j] = ((i + j) & 0x0F) * 0x1P-4;
                        mat_b[i][j] = (((i << 1) + (j >> 1)) & 0x0F) * 0x1P-4;
                }
        }

        memset(mat_c, 0, sizeof(mat_c));
        memset(mat_ref, 0, sizeof(mat_ref));
}

/* 
 * Wrap a sensor device
 */
static void
sense_wrapper()
{

  return 0;
}

/*
 * Update QoS (waketime)
 */
static void
AIMD(*)
{
  return 0;
}

int
main(int argc, char *argv[])
{
        return 0;
}

