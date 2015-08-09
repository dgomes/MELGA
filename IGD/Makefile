CC ?= gcc
#AR = gar
CFLAGS ?= -O
CFLAGS += -Wall
CFLAGS += -D_GNU_SOURCE 
CFLAGS += -DMINIUPNPC_SET_SOCKET_TIMEOUT
CFLAGS += -DMINIUPNPC_GET_SRC_ADDR
CFLAGS += -D_BSD_SOURCE -D_POSIX_C_SOURCE=1
CFLAGS += -DDEBUG

INCLUDE_DIRS ?= -I/usr/include/miniupnpc/ -I. -I..
LIBRARY_DIRS ?= -L/usr/lib/

LDFLAGS = -lminiupnpc -lmosquitto

%.o : %.c
	$(CC) $(CFLAGS) $(INCLUDE_DIRS) -c $< -o $@

igd: igd.o
	$(CC) $(CFLAGS) $(LIBRARY_DIRS) -o $@ $(LDFLAGS) $^ $(LIBRARIES)

clean:
	rm *.o igd
