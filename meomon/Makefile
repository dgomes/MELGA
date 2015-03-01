LIB_MOSQUITTO=-lmosquitto

CFLAGS=-g -std=gnu99 -D_GNU_SOURCE -Wall -O0 #-DDEBUG

CFLAGS+=`xml2-config --cflags`
LIBRARIES=`xml2-config --libs` $(LIB_MOSQUITTO) `pkg-config --libs libconfig`

INCLUDE_DIRS=-I. -I..

SOURCES := $(wildcard *.c)
OBJS    := $(SOURCES:.c=.o) ../utils.o

OS := $(shell uname)
ifeq ($(OS),  Darwin)
# Run MacOS commands 
CFLAGS+=-DOSX
endif


%.o : %.c
	$(CC) $(INCLUDE_DIRS) -c $(CFLAGS) $< -o $@

meomon: $(OBJS)
	$(CC) $(LIBRARY_DIRS) -o $@ $(LDFLAGS) $^ $(LIBRARIES)

clean:
	rm $(OBJS) meomon

all: meomon
