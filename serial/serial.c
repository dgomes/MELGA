#include "serial.h"

int setupSerial(const char *port, int baudrate) {
	int fd = -1;
	DBG("%s\n", port);
	fd = serialport_init(port, baudrate);
	if( fd==-1 ) {
		ERR("couldn't open port %s @ %d bauds\n", port, baudrate);
		return -1;
	}
	DBG("opened port %s @ %d bps\n",port, baudrate);

	serialport_flush(fd);
	return fd;
};

int readSerial(int fd, char *buf, int buf_max, int timeout) {
	char eolchar = '\n';
	int sr = serialport_read_until(fd, buf, eolchar, buf_max, timeout);
	if(buf[0] != eolchar)
		DBG("DEBUG:\t%s", buf);

	return sr;
};

int arduinoEvent(char *buf, config_t *cfg, struct mosquitto *mosq) {
	if(strlen(buf)<2) return ERR_NO_JSON;

	json_error_t error;
	json_t *root = json_loads(buf, 0, &error);

    if(root == NULL) {
        DBG("<%s>\n", buf);
        ERR("error: on line %d: %s\n", error.line, error.text);
		return ERR_NO_JSON;
    }

	void *id = json_object_iter_at(root, "id");
	if(id == NULL) {
		json_decref(root);
		return ERR_NO_ID;
	}
	json_t *json_device = json_object_iter_value(id);
	const char *device = json_string_value(json_device);

	char topic[255];
	snprintf(topic, 255, "%s/raw", device);
	mosquitto_publish(mosq, NULL, topic, strlen(buf), strtok(buf,"\n"), 0, false);

	void *code = json_object_iter_at(root, "code");
	if(code == NULL) {
		json_decref(root);
		return ERR_NO_ID;
	}
	json_t *json_code = json_object_iter_value(code);

	if(json_integer_value(json_code) == 200) {
		time_t now;
		now = time(NULL);
		snprintf(topic, 255, "%s/timestamp", device);
		char payload[20];
		strftime(payload, 20, "%Y-%m-%d %H:%M:%S", localtime(&now));
		mosquitto_publish(mosq, NULL, topic, strlen(payload), payload, 0, true);
	};

	const char *key;
	json_t *value;
	void *iter = json_object_iter(root);
	while(iter) {
		key = json_object_iter_key(iter);
		value = json_object_iter_value(iter);
		/* use key and value ... */
		char payload[2048];
		snprintf(topic, 255,  "%s/%s", device, key);
		if(json_is_integer(value)) {
			snprintf(payload, 2048, "%lld", json_integer_value(value));
		} else if(json_is_real(value)) {
			snprintf(payload, 2048, "%f", json_real_value(value));
		} else if(json_is_boolean(value)) {
			if(json_is_true(value))
				snprintf(payload, 2048, "true");
			else
				snprintf(payload, 2048, "false");
		} else {
			snprintf(payload, 2048, "%s", json_string_value(value));
		}
		mosquitto_publish(mosq, NULL, topic, strlen(payload), payload, 0, true);
//		DBG("%s -> %s\n", topic, payload);
		iter = json_object_iter_next(root, iter);
	}
	json_decref(root);
	return 0;
}

void connect_callback(struct mosquitto *mosq, void *userdata, int level) {
	global_data_t *g = userdata;

	//Subscribe command
	char sub[255];
	snprintf(sub, 255, "%s/cmd", g->client_id);
	int r = mosquitto_subscribe(mosq, NULL, sub, 2);
	DBG("Subscribe %s = %d\n", sub, r);
	if(r != MOSQ_ERR_SUCCESS) {
		ERR("Could not subscribe to %s", sub);
	}
}

void disconnect_callback(struct mosquitto *mosq, void *userdata, int level) {
	/* Pring all log messages regardless of level. */
#ifdef DEBUG_MQTT
	DBG("[MQTT DISCONNECT] %s\n", str);
#endif
}

void log_callback(struct mosquitto *mosq, void *userdata, int level, const char *str) {
	/* Pring all log messages regardless of level. */
#ifdef DEBUG_MQTT
	DBG("[MQTT LOG] %s\n", str);
#endif
}

void message_callback(struct mosquitto *mosq, void *userdata, const struct mosquitto_message *message) {
	DBG("%s -> %s\n", message->topic, (char *) message->payload);

	global_data_t *g = userdata;
	serialport_write(g->arduino_fd, message->payload);
	serialport_write(g->arduino_fd, "\n");

}

int main( int argc, char* argv[] ) {
	config_t cfg;
	global_data_t state;
	struct mosquitto *mosq = NULL;

	config_init(&cfg);
	loadDefaults(&cfg);

	if(parseArgs(argc, argv, &cfg)) {
		exit(1);
	}

	//config_write(&cfg, stderr);

	port_t serial;
	loadSerial(&cfg, &serial);

	if(serial.name == NULL) {
		ERR("Could not load serial configuration\n");
		exit(2);
	}

	if(!strlen(serial.name)) {
		ERR("You must specify the serial port\n");
		exit(2);
	}

	DBG("Setting up serial port...");
	state.arduino_fd = setupSerial(serial.name, serial.speed);
	if(state.arduino_fd < 0) {
		ERR("Failed to setup serial port %s @ %d\n", serial.name, serial.speed);
		exit(2);
	}

	mosquitto_lib_init();
	mqttserver_t mqtt;
	loadMQTT(&cfg, &mqtt);

	char hostname[BUF_MAX];
	gethostname(hostname, BUF_MAX);
	asprintf(&state.client_id, "%s.%s", hostname, (char *) strlaststr(serial.name, "/"));
	mosq = mosquitto_new(state.client_id, true, &state.arduino_fd); //use port name as client id
	if(!mosq) {
		ERR("Couldn't create a new mosquitto client instance\n");
		exit(3);
	}
	INFO("listening for event on %s\n", serial.name);

	//TODO setup callbacks
	mosquitto_log_callback_set(mosq, log_callback);
	mosquitto_disconnect_callback_set(mosq, disconnect_callback);
	mosquitto_connect_callback_set(mosq, connect_callback);
	mosquitto_message_callback_set(mosq, message_callback);

	if(mosquitto_connect(mosq, mqtt.servername, mqtt.port, mqtt.keepalive)){
		ERR("Unable to connect to %s:%d.\n", mqtt.servername, mqtt.port);
		exit(3);
	}
	INFO("connected to %s:%d\n",  mqtt.servername, mqtt.port);

	int mosq_fd = mosquitto_socket(mosq);

	fd_set active_fd_set, read_fd_set;
	/* Initialize the set of active sockets. */
	FD_ZERO (&active_fd_set);
	FD_SET (state.arduino_fd, &active_fd_set);
	FD_SET (mosq_fd, &active_fd_set);

	char buf[BUF_MAX];
	bzero(buf,BUF_MAX);

	int retries = 0;

	//TODO setup syscall to stop process
	while(1) {
		/* Block until input arrives on one or more active sockets. */
		read_fd_set = active_fd_set;
		if (select (FD_SETSIZE, &read_fd_set, NULL, NULL, NULL) < 0) {
			ERR("Error in select\n");
			sleep(BACKOFF);
			int r = mosquitto_reconnect(mosq);
			retries++;
			if(r != MOSQ_ERR_SUCCESS) {
				ERR("Could not reconnect to broker: %s\n", strerror(r));
				if(retries > MAX_RETRIES) {
					/* Cleanup */
					mosquitto_destroy(mosq);
					mosquitto_lib_cleanup();
					exit (EXIT_FAILURE);
				}
			} else {
				retries = 0;
				continue;
			}
		}

		/* Service all the sockets with input pending. */
		int i;
		for (i = 0; i < FD_SETSIZE; ++i)
			if (FD_ISSET (i, &read_fd_set)) {
				if(i == state.arduino_fd) {
					if(!readSerial(state.arduino_fd, buf, BUF_MAX, serial.timeout)) {
						arduinoEvent(buf, &cfg, mosq);
						bzero(buf,BUF_MAX);
					}
				} else if(i == mosq_fd) {
					mosquitto_loop_read(mosq, 1);
					mosquitto_loop_write(mosq, 1);
					mosquitto_loop_misc(mosq);
				}
			}
	};

	return EXIT_SUCCESS;
}
