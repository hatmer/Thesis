CONTIKI_PROJECT = udp-client udp-server
all: $(CONTIKI_PROJECT)

PROJECT_SOURCEFILES += energest.c miti.c

CONTIKI=../..
include $(CONTIKI)/Makefile.include
