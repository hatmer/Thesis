CC=gcc
CFLAGS:=-Werror -O3 -g $(CFLAGS)

all: miti

multiply: muti.o

clean:	
	rm -f muti *.o
