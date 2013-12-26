LIBRARY_DIRS= -L /usr/local/lib
LIBRARIES=-lmosquitto libxively/obj/libxively.a
INCLUDE_DIRS=-I libxively/src/libxively

CFLAGS=-g

SOURCES := $(wildcard *.c)
OBJS	:= $(SOURCES:.c=.o)

%.o : %.c
	$(CC) $(INCLUDE_DIRS) -c $(CFLAGS) $< -o $@

xively: xively.o
	$(CC) $(LIBRARY_DIRS) -o $@ $(LDFLAGS) $^ $(LIBRARIES)

clean:
	rm $(OBJS) xively
