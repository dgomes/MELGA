ARDUINO_SERIAL_INCLUDE_DIR=-I arduino-serial/
LIB_ARDUINO_SERIAL=arduino-serial/arduino-serial-lib.o


LIBRARIES=$(LIB_XIVELY) $(LIB_ARDUINO_SERIAL) $(LIB_MOSQUITTO) `pkg-config --libs jansson`
INCLUDE_DIRS=-I. -I.. $(XIVELY_INCLUDE_DIR) $(ARDUINO_SERIAL_INCLUDE_DIR) `pkg-config --cflags jansson`

CFLAGS?= -std=gnu99 -D_GNU_SOURCE -Wall
CFLAGS+=-g -O0 -DDEBUG
#CFLAGS+=-O3 #-DDEBUG=1

SOURCES	:= $(wildcard *.c)
OBJS	:= $(SOURCES:.c=.o) ../utils.o ../config.o

include ../mosquitto.mk
include ../config.mk

%.o : %.c
	$(CC) $(INCLUDE_DIRS) -c $(CFLAGS) $< -o $@

serial: $(OBJS)
	$(CC) $(LIBRARY_DIRS) -o $@ $(LDFLAGS) $^ $(LIBRARIES)

clean:
	rm $(OBJS) serial

all: serial
