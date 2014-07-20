#ifndef _CONFIG_H_
#define _CONFIG_H_

#include <getopt.h>
#include <stdio.h>

#include <jansson.h>
#include <string.h>
#include <utils.h>

typedef struct {
	struct _port {
		char *name;
		int speed;
		int timeout; // mili seconds
	} port;
	struct _server {
		char *servername;
		int port;
		int keepalive;
	} remote;
	char *conffile;
} config_t;

int loadDefaults(config_t *c);
int parseArgs(int argc, char *argv[], config_t *c);
int readConfig(const char *filename, config_t *c);
char *dumpConfig(config_t *c);

#endif
