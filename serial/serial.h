#ifndef _SERIAL_H_
#define _SERIAL_H_

#include <arduino-serial-lib.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>
#include <mosquitto.h>

//OSX
#include <sys/select.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <unistd.h>

#include <utils.h>
#include "config.h"

#define ERR_NO_JSON 1
#define ERR_NO_ID 2 

#define BACKOFF 1 //seconds
#define MAX_RETRIES 60 

#define BUF_MAX  512

int setupSerial(const char *port, int baudrate);
int readSerial(int fd, char *buf, int buf_max, int timeout);

typedef struct {
	int arduino_fd;
	char *client_id;
} global_data_t;
#endif
