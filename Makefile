CONTIKI_PROJECT = udp-client udp-server
all: $(CONTIKI_PROJECT)

CONTIKI_WITH_RPL=0

PROJECT_SOURCEFILES += energest.c miti.c

CONTIKI=../..
include $(CONTIKI)/Makefile.include
