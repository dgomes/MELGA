#ifndef _XIVELY_H_
#define _XIVELY_H_

#include <inttypes.h>
#include <string.h>
#include <time.h>
#include <stdio.h>

#include <libconfig.h>
#include <xi_debug.h>
#include <xively.h>
#include <xi_helpers.h>
#include <xi_err.h>
#include <mosquitto.h>
#include <syslog.h>
#include <utils.h>
#include "../utils.h"

typedef struct {
	xi_feed_t f;
	const char* topic;
	unsigned updates;
	const char* apikey;
} feed_t;

typedef struct {
	feed_t **feeds;
	unsigned n_feeds;
	unsigned last_feed;
	config_t cfg;
} data_t;

void free_feed(feed_t *);
int create_feed(data_t *data, const char *topic);
int exist_feed_topic(data_t *data, const char *topic);

void message_callback(struct mosquitto *mosq, void *userdata, const struct mosquitto_message *message);
void connect_callback(struct mosquitto *mosq, void *userdata, int result);
void log_callback(struct mosquitto *mosq, void *userdata, int level, const char *str);

int setup(struct mosquitto *mosq, config_t *cfg, const char *conf_filename); 
void configure(struct mosquitto *mosq, data_t *data);

#endif //_XIVELY_H_
