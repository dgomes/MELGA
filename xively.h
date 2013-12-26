#ifndef _XIVELY_H_
#define _XIVELY_H_

#include <inttypes.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdio.h>

#include <xi_debug.h>
#include <xively.h>
#include <xi_helpers.h>
#include <xi_err.h>
#include <mosquitto.h>

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

feed_t *init_feed(const char* topic, int xi_feed_id);
void free_feed(feed_t *);
int updateFeed(char *ApiKey, int feed_id, feed_t *feed);


#endif //_XIVELY_H_
