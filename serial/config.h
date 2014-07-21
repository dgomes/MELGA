#ifndef _CONFIG_H_
#define _CONFIG_H_

#include <getopt.h>
#include <stdio.h>

#include <libconfig.h>
#include <jansson.h>
#include <string.h>

#include <utils.h>

typedef	struct {
	const char *name;
	int speed;
	int timeout; // mili seconds
} port_t;

typedef struct {
	const char *servername;
	int port;
	int keepalive;
} mqttserver_t;

int loadDefaults(config_t *c);
int parseArgs(int argc, char *argv[], config_t *c);
int loadSerial(config_t *c, port_t *port);
int loadMQTT(config_t *c, mqttserver_t *mqtt);

#endif
