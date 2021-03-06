#include "xively.h"

int create_feed(data_t *data, const char *topic) {
	/*configure feed/datastreams */
	data->n_feeds++;
	data->feeds = realloc(data->feeds, data->n_feeds * sizeof(feed_t *));
	unsigned new_feed = data->n_feeds-1;

	{ /*INIT FEED */
	feed_t *feed = (feed_t *) malloc(sizeof(feed_t));
	feed->topic = strdup(topic);
	memset(&(feed->f) , 0, sizeof( xi_feed_t ) );
	/* set feed properties */
	feed->f.datastream_count = 0;
	feed->apikey = NULL;
	feed->updates = 0;

	data->feeds[new_feed] = feed;
	}

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
		i = (i+1) % data->n_feeds; /*circular */
	} while(i!=data->last_feed);
	return -1;
}

int setup(struct mosquitto *mosq, config_t *cfg, const char *conf_filename) {
	config_init(cfg);

	INFO("Read configuration file %s\n", conf_filename);
	/* Read the file. If there is an error, report it and exit. */
	if(!config_read_file(cfg, conf_filename))
	{
		ERR("%s:%d - %s", config_error_file(cfg), config_error_line(cfg), config_error_text(cfg));
		config_destroy(cfg);
		exit(EXIT_FAILURE);
	}
	return 0;
}

void configure(struct mosquitto *mosq, data_t *data) {

	config_t *cfg = &data->cfg;
	config_setting_t *setting;

	setting = config_lookup(cfg, "devices");
	if(setting != NULL) {
		for(int i = 0; i<config_setting_length(setting); i++) {
			config_setting_t *device = config_setting_get_elem(setting, i);

			const char *name, *apikey;
			int feedid;
			config_setting_t *channels;

			if(!(config_setting_lookup_string(device, "name", &name)
				&& config_setting_lookup_int(device, "feedid", &feedid)
				&& config_setting_lookup_string(device, "apikey", &apikey)
				&& (channels = config_setting_get_member(device, "channels"))
			))
				continue;

			int feed_i = create_feed(data, name);

			data->feeds[feed_i]->f.feed_id = feedid;
			DBG("%s feedid = %d\n", data->feeds[feed_i]->topic, feedid);

			data->feeds[feed_i]->apikey = strdup(apikey);
			DBG("%s apikey = %s\n", data->feeds[feed_i]->topic, apikey);


			for (int j=0; j<config_setting_length(channels); j++) {
				config_setting_t *channel = config_setting_get_elem(channels, j);
				char *datastream = strdup(config_setting_get_string(channel));

				char *sub;
				asprintf(&sub, "%s/%s", data->feeds[feed_i]->topic, datastream);
				int r = mosquitto_subscribe(mosq, NULL, sub, 2);
				DBG("Subscribe %s = %d\n", sub, r);
				if(r != MOSQ_ERR_SUCCESS) {
					ERR("Could not subscribe to %s\n", sub);
					ERR("%s\n", mosquitto_strerror(r));
					exit(EXIT_FAILURE);
				}
				free(sub);

				xi_datastream_t* d	= &data->feeds[feed_i]->f.datastreams[data->feeds[feed_i]->f.datastream_count];
				xi_str_copy_untiln(d->datastream_id, sizeof( d->datastream_id ), datastream, '\0');
				data->feeds[feed_i]->f.datastream_count++;
				free(datastream);
			} /* channels */
		} /* device */
	}
	config_destroy(cfg);
}

void message_callback(struct mosquitto *mosq, void *userdata, const struct mosquitto_message *message) {
	data_t *data = userdata; /* get feeds array back */

	int feed_i = exist_feed_topic(data, message->topic);

	feed_t **feeds = data->feeds;
	if(feed_i == -1) {
		ERR("Feed for topic \"%s\" hasn't been created (yet)", message->topic);
		return;
	}
	unsigned updated = 0;
	char *datastream = strchr(message->topic, '/')+1; /* 1 to skip / */
	for(unsigned i=0; i<feeds[feed_i]->f.datastream_count; i++) {
		xi_datastream_t* d  = &data->feeds[feed_i]->f.datastreams[i];

		if(d->datapoint_count) updated++;
		if(!strncmp(datastream, d->datastream_id, sizeof(d->datastream_id)-1)) {
			d->datapoint_count = 1;
			updated++;
			xi_datapoint_t *p = &d->datapoints[0];
			memset(p, 0, sizeof(xi_datapoint_t));

			xi_set_value_str(p, message->payload);
#if 0
			/*test if it is an INT or FLOAT */
			char *endptr;
			intmax_t num = strtoimax(message->payload, &endptr, 10);
			if(num == INTMAX_MAX || num == INTMAX_MIN || *endptr != '\0') {
				/*it's a float or string ? */
				float numf = strtof(message->payload, &endptr);
				if(*endptr == '\0') {
					xi_set_value_f32(p, numf);
		//			DBG("%s -> %f\n", d->datastream_id, strtof(message->payload, NULL));
				} else {
					/*we don't publish strings to xively... */
					d->datapoint_count = 0;
				}
			} else {
				/*it's an int */
				xi_set_value_i32(p, num);
		//		DBG("%s -> %ld\n", d->datastream_id, (long int) num);
			}
#endif
		}
	}
	/*Have we received updates on all datastreams? */
//	DBG("updated = %d == feeds[feed_i]->f.datastream_count = %d\n", updated, feeds[feed_i]->f.datastream_count);
	if(updated == feeds[feed_i]->f.datastream_count) {
		if(feeds[feed_i]->apikey == NULL) {
			ERR("Failed to publish to xively.com - API Key missing for feed %s", feeds[feed_i]->topic);
			return;
		}

		xi_context_t* xi_context = xi_create_context( XI_HTTP, feeds[feed_i]->apikey , feeds[feed_i]->f.feed_id);
		/* check if everything works */
		if( xi_context == 0 ) {
			ERR("Error creating xi_context");
			return;
		}

		xi_feed_update( xi_context, &feeds[feed_i]->f );

		if(( int ) xi_get_last_error() > 0) {
			ERR("err: %d - %s", ( int ) xi_get_last_error(), xi_get_error_string( xi_get_last_error()));
		} else {
			DBG("Published <%s> to feed_id:%u\n", feeds[feed_i]->topic, feeds[feed_i]->f.feed_id);
		}
		/* destroy the context cause we don't need it anymore */
		xi_delete_context( xi_context );

		/* Erase everything */
		for(unsigned i=0; i<feeds[feed_i]->f.datastream_count; i++) {
			xi_datastream_t* d  = &data->feeds[feed_i]->f.datastreams[i];
			d->datapoint_count = 0;
			xi_datapoint_t *p = &d->datapoints[0];
			memset(p, 0, sizeof(xi_datapoint_t));
		}

	}
}

void disconnect_callback(struct mosquitto *mosq, void *userdata, int result) {
	if(!result){
		NOTICE("Disconnected from MQTT Broker");
	}else{
		ERR("Unexpectably disconnected from broker");
	}
}

void connect_callback(struct mosquitto *mosq, void *userdata, int result) {
	if(!result){
		NOTICE("Connected to MQTT Broker");
		configure(mosq, userdata);
	}else{
		ERR("Connection to broker failed");
	}
}

void log_callback(struct mosquitto *mosq, void *userdata, int level, const char *str) {
	/* Pring all log messages regardless of level. */
	#ifdef DEBUG_MQTT
	DBG("[MQTT LOG] %s\n", str);
	#endif
}

int main(int argc, char *argv[]) {
	char id[30] = "xively"; //TODO randomize this else broker will keep us disconnecting...
	DBG("Xively Publisher v1\n");
	//TODO read this from the configuration file
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

	setup(mosq, &data.cfg, "/home/dgomes/MELGA/xively/xively.cfg");

	mosq = mosquitto_new(id, clean_session, &data);

	if(!mosq){
		ERR("Error: Out of memory.");
		exit(EXIT_FAILURE);
	}

	mosquitto_connect_callback_set(mosq, connect_callback);
	mosquitto_disconnect_callback_set(mosq, disconnect_callback);
	mosquitto_message_callback_set(mosq, message_callback);
	mosquitto_log_callback_set(mosq, log_callback);

	if(mosquitto_connect(mosq, host, port, keepalive)){
		ERR("Unable to connect.");
		return(EXIT_FAILURE);
	}

	NOTICE("Xively Publisher started");
	/* do anything besides waiting for new values to publish, so lets loop_forever */
	int r = mosquitto_loop_forever(mosq, -1, 1);
	if(r != MOSQ_ERR_SUCCESS) {
		ERR("%s\n", mosquitto_strerror(r));
		exit(EXIT_FAILURE);
	}
	DBG("loop_forever exited!?\n");

	/* Cleanup */
	mosquitto_destroy(mosq);
	mosquitto_lib_cleanup();

	for(unsigned i=0; i<data.n_feeds; i++)
		free_feed(data.feeds[i]);
	free(data.feeds);

	NOTICE("shutdown completed");
	closelog();
	exit(EXIT_SUCCESS);
}
