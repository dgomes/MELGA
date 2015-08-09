#ifndef _SERIALCONFIG_H_
#define _SERIALCONFIG_H_

#include <getopt.h>
#include <stdio.h>

#include <libconfig.h>
#include <string.h>
#include <jansson.h>

#include <utils.h>

#define CONFIG_SERIAL			"serial"
#define CONFIG_SERIAL_DEVNAME	"devname"
#define CONFIG_SERIAL_SPEED		"speed"
#define CONFIG_SERIAL_TIMEOUT	"timeout"

#define DEFAULT_SERIAL_SPEED	115200
#define DEFAULT_SERIAL_TIMEOUT	30000

typedef	struct {
	const char *name;
	int speed;
	int timeout; // mili seconds
} port_t;

int parseSerialArgs(int argc, char *argv[], config_t *c);
int loadSerialDefaults(config_t *c);
int loadSerial(config_t *c, port_t *port);

#endif
