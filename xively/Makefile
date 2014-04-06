LIBRARY_DIRS= -L /usr/local/lib
LIBRARIES=`pkg-config --libs libconfig` -lmosquitto libxively/obj/libxively.a
INCLUDE_DIRS=-I libxively/src/libxively

CFLAGS=-g -std=gnu99 -Wall -DDEBUG=1 `pkg-config --cflags libconfig`

SOURCES := $(wildcard *.c)
OBJS	:= $(SOURCES:.c=.o)

%.o : %.c
	$(CC) $(CFLAGS) $(INCLUDE_DIRS) -c $< -o $@

xively: xively.o
	$(CC) $(CFLAGS) $(LIBRARY_DIRS) -o $@ $(LDFLAGS) $^ $(LIBRARIES)

clean:
	rm $(OBJS) xively
