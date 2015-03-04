ARDUINO_SERIAL_INCLUDE_DIR=-I arduino-serial/
LIB_ARDUINO_SERIAL=arduino-serial/arduino-serial-lib.o

LIB_MOSQUITTO=-lmosquitto

LIBRARIES=$(LIB_XIVELY) $(LIB_ARDUINO_SERIAL) $(LIB_MOSQUITTO) `pkg-config --libs jansson` `pkg-config --libs libconfig`
INCLUDE_DIRS=-I. -I.. $(XIVELY_INCLUDE_DIR) $(ARDUINO_SERIAL_INCLUDE_DIR) `pkg-config --cflags jansson`

CFLAGS?= -std=gnu99 -D_GNU_SOURCE -Wall `pkg-config --cflags libconfig`
#CFLAGS+=-g -O0
CFLAGS+=-O3 #-DDEBUG=1

SOURCES	:= $(wildcard *.c)
OBJS	:= $(SOURCES:.c=.o) ../utils.o

%.o : %.c
	$(CC) $(INCLUDE_DIRS) -c $(CFLAGS) $< -o $@

serial: $(OBJS)
	$(CC) $(LIBRARY_DIRS) -o $@ $(LDFLAGS) $^ $(LIBRARIES)

clean:
	rm $(OBJS) serial

all: serial
