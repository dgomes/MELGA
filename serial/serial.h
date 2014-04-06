#ifndef _SERIAL_H_
#define _SERIAL_H_

#include <arduino-serial-lib.h>

//#define DEBUG

#include "utils.h"

#define BUF_MAX  512
int setupSerial(char *port, int baudrate);
int readSerial(int fd, char *buf, int buf_max, int timeout);

typedef struct {
	char *buf;
	time_t time;
} last_update_t;
#endif
