#ifndef _CONFIG_H_
#define _CONFIG_H_

#include <getopt.h>
#include <stdio.h>

#include <libconfig.h>
#include <jansson.h>
#include <string.h>

#include <utils.h>

#define CONFIG_SERIAL			"serial"
#define CONFIG_SERIAL_DEVNAME	"devname"
#define CONFIG_SERIAL_SPEED		"speed"
#define CONFIG_SERIAL_TIMEOUT	"timeout"

#define CONFIG_MQTT				"mqtt"
#define CONFIG_MQTT_SERVER		"server"
#define CONFIG_MQTT_PORT		"port"
#define CONFIG_MQTT_KEEP_ALIVE	"keep_alive"

#define DEFAULT_SERIAL_SPEED	115200
#define DEFAULT_SERIAL_TIMEOUT	30000

#define DEFAULT_MQTT_SERVER	"localhost"
#define DEFAULT_MQTT_KEEP_ALIVE 	1800
#define DEFAULT_MQTT_SERVER_PORT	1883

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
