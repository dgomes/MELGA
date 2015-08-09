SUBDIRS := meomon IGD serial
CLEANDIRS = $(SUBDIRS:%=clean-%)

LIBRARIES= `pkg-config --libs libconfig` # `pkg-config --libs jansson` 
INCLUDE_DIRS=-I. -I.. #`pkg-config --cflags jansson`

CFLAGS?= -std=gnu99 -D_GNU_SOURCE -Wall `pkg-config --cflags libconfig`
#CFLAGS+=-g -O0
CFLAGS+=-O3 #-DDEBUG=1

SOURCES	:= $(wildcard *.c)
OBJS	:= $(SOURCES:.c=.o) 

.PHONY: subdirs all clean $(SUBDIRS)

all: $(OBJS) $(SUBDIRS)

%.o : %.c
	$(CC) $(INCLUDE_DIRS) -c $(CFLAGS) $< -o $@

subdirs: $(SUBDIRS)
$(SUBDIRS): 
	$(MAKE) -C $@

clean: $(CLEANDIRS)
	rm *.o 
$(CLEANDIRS):
	$(MAKE) -C $(@:clean-%=%) clean


