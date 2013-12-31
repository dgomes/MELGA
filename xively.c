#include "xively.h"

int create_feed(data_t *data, const char *topic) {
	//configure feed/datastreams
	data->n_feeds++;
	data->feeds = realloc(data->feeds, data->n_feeds * sizeof(feed_t *));
	unsigned new_feed = data->n_feeds-1;

	char *t_free = strdup(topic);
	char *t = t_free;
	char *feed_topic = strsep(&t, "/");

	{ //INIT FEED
	feed_t *feed = (feed_t *) malloc(sizeof(feed_t));
	feed->topic = strdup(feed_topic);
	memset(&(feed->f) , 0, sizeof( xi_feed_t ) );
	// set feed properties
	feed->f.datastream_count = 0;
	feed->apikey = NULL;
	feed->updates = 0;

	data->feeds[new_feed] = feed;
	}

	free(t_free);

	return new_feed;
}

void free_feed(feed_t *feed) {
	free((void *)feed->apikey);
	free((void *)feed->topic);
	free((void *)feed);
};


int exist_feed_topic(data_t *data, const char *topic) {
	feed_t **feeds = data->feeds;
	if(feeds == NULL) return -1;

	unsigned i = data->last_feed;
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

			DBG("%s feedid = %s\n", data->feeds[feed_i]->topic, (char *) message->payload);
			return;
		}

		if(strstr(message->topic, "/xively/apikey") != NULL) {
			data->feeds[feed_i]->apikey = strdup(message->payload);

			DBG("%s apikey = %s\n", data->feeds[feed_i]->topic, (char *) message->payload);
			return;
		}

		if(strstr(message->topic, "/xively/datastream") != NULL) {
			char *datastream;
			char *ds = strdup(message->payload);
			data->feeds[feed_i]->f.datastream_count = 0;
			while ((datastream = strsep(&ds, " ,")) != NULL) {
				if(!strlen(datastream)) continue;
				char *sub = (char *) malloc(strlen(data->feeds[feed_i]->topic) + 1 + strlen(datastream)+1);
					sprintf(sub, "%s/%s", data->feeds[feed_i]->topic, datastream);
				mosquitto_subscribe(mosq, NULL, sub, 2);
				free(sub);
				xi_datastream_t* d	= &data->feeds[feed_i]->f.datastreams[data->feeds[feed_i]->f.datastream_count];
				xi_str_copy_untiln(d->datastream_id, sizeof( d->datastream_id ), datastream, '\0');
				DBG("%s\n", d->datastream_id);
				data->feeds[feed_i]->f.datastream_count++;
			}
			free(ds);

			DBG("%s datastreams = %s\n", data->feeds[feed_i]->topic, (char *) message->payload);
			return;
		}
	}

	feed_t **feeds = data->feeds;
	if(feed_i == -1) {
		ERR("Feed for topic \"%s\" hasn't been created (yet)", message->topic);
		return;
	}
	unsigned updated = 0;
	char *datastream = strchr(message->topic, '/')+1; // 1 to skip /
	for(unsigned i=0; i<feeds[feed_i]->f.datastream_count; i++) {
		xi_datastream_t* d  = &data->feeds[feed_i]->f.datastreams[i];

		if(d->datapoint_count) updated++;
		if(!strncmp(datastream, d->datastream_id, sizeof(d->datastream_id)-1)) {
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
					DBG("%s -> %f\n", d->datastream_id, strtof(message->payload, NULL));
				} else {
					//we don't publish strings to xively...
					d->datapoint_count = 0;
				}
			} else {
				//it's an int
				xi_set_value_i32(p, num);
				DBG("%s -> %ld\n", d->datastream_id, (long int) num);
			}
		}
	}
	//Have we received updates on all datastreams?
	if(updated == feeds[feed_i]->f.datastream_count) {
		DBG("SEND TO XIVELY!\n");

		if(feeds[feed_i]->apikey == NULL) {
			ERR("Failed to publish to xively.com - API Key missing for feed %s", feeds[feed_i]->topic);
			return;
		}

		xi_context_t* xi_context = xi_create_context( XI_HTTP, feeds[feed_i]->apikey , feeds[feed_i]->f.feed_id);
		// check if everything works
		if( xi_context == 0 ) {
			ERR("Error creating xi_context");
			return;
		}

		xi_feed_update( xi_context, &feeds[feed_i]->f );

		if(( int ) xi_get_last_error() > 0)
			ERR("err: %d - %s", ( int ) xi_get_last_error(), xi_get_error_string( xi_get_last_error()));
		else
			INFO("Published <%s> to feed_id:%u", feeds[feed_i]->topic, feeds[feed_i]->f.feed_id);
		// destroy the context cause we don't need it anymore
		xi_delete_context( xi_context );

		// Erase everything
		for(unsigned i=0; i<feeds[feed_i]->f.datastream_count; i++) {
			xi_datastream_t* d  = &data->feeds[feed_i]->f.datastreams[i];
			d->datapoint_count = 0;
			xi_datapoint_t *p = &d->datapoints[0];
			memset(p, 0, sizeof(xi_datapoint_t));
		}

	}
}

void connect_callback(struct mosquitto *mosq, void *userdata, int result) {
	if(!result){
		/* Subscribe to broker information topics on successful connect. */
		NOTICE("Connected");
		mosquitto_subscribe(mosq, NULL, "+/xively/#", 2);
	}else{
		ERR("Connection to broker failed");
	}
}

void log_callback(struct mosquitto *mosq, void *userdata, int level, const char *str) {
	/* Pring all log messages regardless of level. */
	DBG("[MQTT LOG] %s\n", str);
}

void daemonize() {
	#ifndef DEBUG
	/* Our process ID and Session ID */
	pid_t pid, sid;

	/* Fork off the parent process */
	pid = fork();
	if (pid < 0) {
		exit(EXIT_FAILURE);
	}
	/* If we got a good PID, then
	   we can exit the parent process. */
	if (pid > 0) {
		exit(EXIT_SUCCESS);
	}

	/* Change the file mode mask */
	umask(0);

	/* Create a new SID for the child process */
	sid = setsid();
	if (sid < 0) {
		/* Log the failure */
		ERR("Failed to create a new SID");
		exit(EXIT_FAILURE);
	}

	/* Change the current working directory */
        if ((chdir("/tmp")) < 0) {
                /* Log the failure */
                exit(EXIT_FAILURE);
        }

        /* Close out the standard file descriptors */
	close(STDIN_FILENO);
        close(STDOUT_FILENO);
        close(STDERR_FILENO);
	#endif
}

int main(int argc, char *argv[]) {
	char id[30] = "xively";
	char *host = "192.168.1.10";
	int port = 1883;
	int keepalive = 60;
	bool clean_session = true;
	struct mosquitto *mosq = NULL;

	/* Open any logs here */
	setlogmask (LOG_UPTO (LOG_INFO));
	openlog(id, LOG_CONS | LOG_PID | LOG_NDELAY, LOG_LOCAL1);

	daemonize();

	mosquitto_lib_init();

	data_t data;
	data.last_feed = 0;
	data.n_feeds = 0;
	data.feeds = NULL;

	mosq = mosquitto_new(id, clean_session, &data);
	if(!mosq){
		ERR("Error: Out of memory.");
		exit(EXIT_FAILURE);
	}

	mosquitto_connect_callback_set(mosq, connect_callback);
	mosquitto_message_callback_set(mosq, message_callback);
	mosquitto_log_callback_set(mosq, log_callback);

	if(mosquitto_connect(mosq, host, port, keepalive)){
		ERR("Unable to connect.");
		return(EXIT_FAILURE);
	}

	//we don't do anything besides waiting for new values to publish, so lets loop_forever
	mosquitto_loop_forever(mosq, -1, 1);

	/* Cleanup */
	mosquitto_destroy(mosq);
	mosquitto_lib_cleanup();
	for(unsigned i=0; i<data.n_feeds; i++)
		free_feed(data.feeds[i]);

	NOTICE("shutdown completed");
	closelog();
	exit(EXIT_SUCCESS);
}
