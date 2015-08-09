CFLAGS ?= -O -Wall
CFLAGS += -std=gnu99 -D_GNU_SOURCE
CFLAGS += -DMINIUPNPC_SET_SOCKET_TIMEOUT -DMINIUPNPC_GET_SRC_ADDR
#CFLAGS += -D_BSD_SOURCE -D_POSIX_C_SOURCE
CFLAGS += -DDEBUG

INCLUDE_DIRS ?= -I/usr/include/miniupnpc -I/usr/local/include/miniupnpc -I. -I..
LIBRARY_DIRS ?= -L/usr/lib/

LDFLAGS = -lminiupnpc

include ../mosquitto.mk
include ../config.mk

%.o : %.c
	$(CC) $(CFLAGS) $(INCLUDE_DIRS) -c $< -o $@

igd: igd.o 
	$(CC) $(CFLAGS) $(LIBRARY_DIRS) -o $@ $(LDFLAGS) $^ $(LIBRARIES)

clean:
	rm *.o igd
