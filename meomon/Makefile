LIB_MOSQUITTO=-lmosquitto

CFLAGS=-g -std=gnu99 -D_GNU_SOURCE -Wall -O0 -DDEBUG=0

CFLAGS+=`xml2-config --cflags`
LIBRARIES=`xml2-config --libs` $(LIB_MOSQUITTO) `pkg-config --libs libconfig`

INCLUDE_DIRS=-I. -I..

SOURCES := $(wildcard *.c)
OBJS    := $(SOURCES:.c=.o) ../utils.o


%.o : %.c
	$(CC) $(INCLUDE_DIRS) -c $(CFLAGS) $< -o $@

main: $(OBJS)
	$(CC) $(LIBRARY_DIRS) -o $@ $(LDFLAGS) $^ $(LIBRARIES)

clean:
	rm $(OBJS) main

all: main
