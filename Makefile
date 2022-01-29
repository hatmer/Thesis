CONTIKI_PROJECT = udp-client udp-server
all: $(CONTIKI_PROJECT)

PROJECT_SOURCEFILES += miti.c sensors.c pir-sensor.c simEnvChange.c energest.c

CONTIKI=../..
include $(CONTIKI)/Makefile.include

