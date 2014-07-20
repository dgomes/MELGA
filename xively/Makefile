LIBRARY_DIRS= -L /usr/local/lib
LIBRARIES=`pkg-config --libs libconfig` -lmosquitto libxively/obj/libxively.a
INCLUDE_DIRS=-I.. -I libxively/src/libxively

CFLAGS=-g -std=gnu99 -Wall `pkg-config --cflags libconfig` #-DDEBUG=1

SOURCES := $(wildcard *.c) $(wildcard ../*.c)
OBJS	:= $(SOURCES:.c=.o)

%.o : %.c
	$(CC) $(CFLAGS) $(INCLUDE_DIRS) -c $< -o $@

xively: $(OBJS)
	$(CC) $(CFLAGS) $(LIBRARY_DIRS) -o $@ $(LDFLAGS) $^ $(LIBRARIES)

clean:
	rm $(OBJS) xively
