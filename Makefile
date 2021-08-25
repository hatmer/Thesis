CONTIKI_PROJECT = udp-client udp-server
all: $(CONTIKI_PROJECT)

PROJECT_SOURCEFILES += miti.c

CONTIKI=../..
include $(CONTIKI)/Makefile.include
