ARDUINO_SERIAL_INCLUDE_DIR=-I arduino-serial/
LIB_ARDUINO_SERIAL=arduino-serial/arduino-serial-lib.o

LIB_MOSQUITTO=-lmosquitto

LIBRARIES=$(LIB_XIVELY) $(LIB_ARDUINO_SERIAL) $(LIB_MOSQUITTO) `pkg-config --libs jansson`
INCLUDE_DIRS=-I. -I.. $(XIVELY_INCLUDE_DIR) $(ARDUINO_SERIAL_INCLUDE_DIR) `pkg-config --cflags jansson`

CFLAGS=-g -std=gnu99 -D_GNU_SOURCE -Wall -O0

SOURCES	:= $(wildcard *.c)
OBJS	:= $(SOURCES:.c=.o) ../utils.o

%.o : %.c
	$(CC) $(INCLUDE_DIRS) -c $(CFLAGS) $< -o $@

serial: $(OBJS)
	$(CC) $(LIBRARY_DIRS) -o $@ $(LDFLAGS) $^ $(LIBRARIES)

clean:
	rm $(OBJS) serial

all: serial
