CC ?= gcc
#AR = gar
#CFLAGS = -O -g -DDEBUG
CFLAGS ?= -O
CFLAGS += -Wall
CFLAGS += -W -Wstrict-prototypes
CFLAGS += -fno-common
CFLAGS += -DMINIUPNPC_SET_SOCKET_TIMEOUT
CFLAGS += -DMINIUPNPC_GET_SRC_ADDR
CFLAGS += -D_BSD_SOURCE -D_POSIX_C_SOURCE=1

INCLUDE_DIRS ?= -I/usr/local/include/miniupnpc/
LIBRARY_DIRS ?= -L/usr/local/lib/

LDFLAGS = -lminiupnpc

%.o : %.c
	$(CC) $(CFLAGS) $(INCLUDE_DIRS) -c $< -o $@

igd: igd.o
	$(CC) $(CFLAGS) $(LIBRARY_DIRS) -o $@ $(LDFLAGS) $^ $(LIBRARIES)

clean:
	rm *.o igd
