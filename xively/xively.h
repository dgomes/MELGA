#ifndef _XIVELY_H_
#define _XIVELY_H_

#include <inttypes.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdio.h>

#ifndef DEBUG
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#endif

#include <libconfig.h>
#include <syslog.h>
#include <xi_debug.h>
#include <xively.h>
#include <xi_helpers.h>
#include <xi_err.h>
#include <mosquitto.h>

#include <utils.h>

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
} data_t;

void free_feed(feed_t *);
int create_feed(data_t *data, const char *topic);
int exist_feed_topic(data_t *data, const char *topic);

void message_callback(struct mosquitto *mosq, void *userdata, const struct mosquitto_message *message);
void connect_callback(struct mosquitto *mosq, void *userdata, int result);
void log_callback(struct mosquitto *mosq, void *userdata, int level, const char *str);

#endif //_XIVELY_H_
