CFLAGS=-g -std=gnu99 -D_GNU_SOURCE -Wall -O0 #-DDEBUG

CFLAGS+=`xml2-config --cflags`
LIBRARIES=`xml2-config --libs`

INCLUDE_DIRS=-I. -I..

SOURCES := $(wildcard *.c)
OBJS    := $(SOURCES:.c=.o) 

include ../mosquitto.mk
include ../os.mk
include ../config.mk

%.o : %.c
	$(CC) $(INCLUDE_DIRS) -c $(CFLAGS) $< -o $@

meomon: $(OBJS)
	$(CC) $(LIBRARY_DIRS) -o $@ $(LDFLAGS) $^ $(LIBRARIES)

clean:
	rm $(OBJS) meomon

all: meomon
