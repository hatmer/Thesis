CONTIKI_PROJECT = udp-client udp-server
all: $(CONTIKI_PROJECT)

DEPS=./miti.h
# gcc file.c -lmiti.h
CONTIKI=../..
include $(CONTIKI)/Makefile.include
