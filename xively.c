#include "xively.h"

feed_t *init_feed(const char* topic, int xi_feed_id) {
	feed_t *feed = (feed_t *) malloc(sizeof(feed_t));

	feed->topic = strdup(topic);
	memset(&(feed->f) , 0, sizeof( xi_feed_t ) );
	// set datastream count
	feed->f.feed_id = xi_feed_id;
	feed->f.datastream_count = 0;
	feed->apikey = NULL;

	feed->updates = 0;

	return feed;
};

void free_feed(feed_t *feed) {
	free((void *)feed->topic);
	free((void *)feed);
};

int updateFeed(char *ApiKey, int feed_id, feed_t *feed) {
	// create the xi library context
	xi_context_t* xi_context = xi_create_context( XI_HTTP, ApiKey , feed_id );

	// check if everything works
	if( xi_context == 0 ) {
			return -1;
	}

	xi_feed_update( xi_context, &feed->f );

	if(( int ) xi_get_last_error() > 0)
		printf( "err: %d - %s\n", ( int ) xi_get_last_error(), xi_get_error_string( xi_get_last_error() ) );

	// destroy the context cause we don't need it anymore
	xi_delete_context( xi_context );

	return 0;
}

int create_feed(data_t *data, const char *topic) {
	//configure feed/datastreams
	data->n_feeds++;
	data->feeds = realloc(data->feeds, data->n_feeds * sizeof(feed_t *));

	char *t_free = malloc(strlen(topic));
	char *t = t_free;
	sprintf(t, "%s", topic);
	char *feed_topic = strsep(&t, "/");

	data->feeds[data->n_feeds-1] = init_feed(feed_topic, -1);
	free(t_free);

	return data->n_feeds-1;
}

int exist_feed_topic(data_t *data, const char *topic) {
	feed_t **feeds = data->feeds;
	if(feeds == NULL) return -1;

	int i = data->last_feed;
	do {
		if(!strncmp(topic, feeds[i]->topic, strlen(feeds[i]->topic))) {
			data->last_feed = i;
			return i;
		}
		i = (i+1) % data->n_feeds; //circular
	} while(i!=data->last_feed);
	return -1;
}
void message_callback(struct mosquitto *mosq, void *userdata, const struct mosquitto_message *message) {
	data_t *data = userdata; //get feeds array back

	int feed_i = exist_feed_topic(data, message->topic);

	if(strstr(message->topic, "/xively/") != NULL) {
		if(feed_i == -1) {
			feed_i = create_feed(data, message->topic);
		}

		if(strstr(message->topic, "/xively/feedid") != NULL) {
			data->feeds[feed_i]->f.feed_id = atoi(message->payload);

			fprintf(stderr, "%s feedid = %s\n", data->feeds[feed_i]->topic, message->payload);
			return;
		}

		if(strstr(message->topic, "/xively/apikey") != NULL) {
			data->feeds[feed_i]->apikey = strdup(message->payload);

			fprintf(stderr, "%s apikey = %s\n", data->feeds[feed_i]->topic, message->payload);
			return;
		}

		if(strstr(message->topic, "/xively/datastream") != NULL) {
			char *datastream;
			char *ds = strdup(message->payload);
			data->feeds[feed_i]->f.datastream_count = 0;
			while ((datastream = strsep(&ds, " ,")) != NULL) {
				if(!strlen(datastream)) continue;
				char *sub = (char *) malloc(strlen(data->feeds[feed_i]->topic) + 1 + strlen(datastream));
					sprintf(sub, "%s/%s", data->feeds[feed_i]->topic, datastream);
				mosquitto_subscribe(mosq, NULL, sub, 2);
				free(sub);
				xi_datastream_t* d	= &data->feeds[feed_i]->f.datastreams[data->feeds[feed_i]->f.datastream_count];
				xi_str_copy_untiln(d->datastream_id, sizeof( d->datastream_id ), datastream, '\0');
				data->feeds[feed_i]->f.datastream_count++;
			}
			free(ds);

			fprintf(stderr, "%s datastreams = %s\n", data->feeds[feed_i]->topic, message->payload);
			return;
		}
	}

	feed_t **feeds = data->feeds;
	if(feed_i == -1) {
		fprintf(stderr, "Feed for topic \"%s\" hasn't been created (yet)\n", message->topic);
		return;
	}
	unsigned updated = 0;
	char *datastream = strchr(message->topic, '/')+1; // 1 to skip /
	for(int i=0; i<feeds[feed_i]->f.datastream_count; i++) {
		xi_datastream_t* d  = &data->feeds[feed_i]->f.datastreams[i];

		if(d->datapoint_count) updated++;

		if(!strcmp(datastream, d->datastream_id)) {
			d->datapoint_count = 1;
			updated++;
			xi_datapoint_t *p = &d->datapoints[0];
			memset(p, 0, sizeof(xi_datapoint_t));

			//test if it is an INT or FLOAT
			char *endptr;
			intmax_t num = strtoimax(message->payload, &endptr, 10);
			if(num == INTMAX_MAX || num == INTMAX_MIN || *endptr != '\0') {
				//it's a float or string ?
				float numf = strtof(message->payload, &endptr);
				if(*endptr == '\0') {
					xi_set_value_f32(p, numf);
					fprintf(stderr,"%s -> %f\n", d->datastream_id, strtof(message->payload, NULL));
				} else {
					//we don't publish strings to xively...
					d->datapoint_count = 0;
				}
			} else {
				//it's an int
				xi_set_value_i32(p, num);
				fprintf(stderr, "%s -> %ld\n", d->datastream_id, num);
			}
		}
	}
	//Have we received updates on all datastreams?
	if(updated == feeds[feed_i]->f.datastream_count) {
		fprintf(stderr,"SEND TO XIVELY!\n");

		xi_context_t* xi_context = xi_create_context( XI_HTTP, feeds[feed_i]->apikey , feeds[feed_i]->f.feed_id);
		// check if everything works
		if( xi_context == 0 ) {
			fprintf(stderr, "Error creating xi_context\n");
			return;
		}

		xi_feed_update( xi_context, &feeds[feed_i]->f );

		if(( int ) xi_get_last_error() > 0)
			fprintf(stderr, "err: %d - %s\n", ( int ) xi_get_last_error(), xi_get_error_string( xi_get_last_error() ) );

		// destroy the context cause we don't need it anymore
		xi_delete_context( xi_context );

		// Erase everything
		for(int i=0; i<feeds[feed_i]->f.datastream_count; i++) {
			xi_datastream_t* d  = &data->feeds[feed_i]->f.datastreams[i];
			d->datapoint_count = 0;
			xi_datapoint_t *p = &d->datapoints[0];
			memset(p, 0, sizeof(xi_datapoint_t));
		}

	}
}

void connect_callback(struct mosquitto *mosq, void *userdata, int result)
{
	int i;
	if(!result){
		/* Subscribe to broker information topics on successful connect. */
		mosquitto_subscribe(mosq, NULL, "+/xively/#", 2);
	}else{
		fprintf(stderr, "Connect failed\n");
	}
}

void my_log_callback(struct mosquitto *mosq, void *userdata, int level, const char *str)
{
	/* Pring all log messages regardless of level. */
	fprintf(stderr,"[MQTT LOG] %s\n", str);
}

int main(int argc, char *argv[])
{
	char id[30] = "xively";
	int i;
	char *host = "192.168.1.10";
	int port = 1883;
	int keepalive = 60;
	bool clean_session = true;
	struct mosquitto *mosq = NULL;

	mosquitto_lib_init();

	data_t data;
	data.last_feed = 0;
	data.n_feeds = 0;
	data.feeds = NULL;

	mosq = mosquitto_new(id, clean_session, &data);
	if(!mosq){
		fprintf(stderr, "Error: Out of memory.\n");
		return 1;
	}

	mosquitto_connect_callback_set(mosq, connect_callback);
	mosquitto_message_callback_set(mosq, message_callback);

	if(mosquitto_connect(mosq, host, port, keepalive)){
		fprintf(stderr, "Unable to connect.\n");
		return 1;
	}

	while(!mosquitto_loop(mosq, -1, 1)){
	}
	mosquitto_destroy(mosq);
	mosquitto_lib_cleanup();
	for(int i=0; i<data.n_feeds; i++)
		free_feed(data.feeds[i]);
	return 0;
}
