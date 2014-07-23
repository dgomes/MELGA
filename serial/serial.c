#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <arduino-serial-lib.h>
#include <time.h>
#include <mosquitto.h>

//OSX
#include <sys/select.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <unistd.h>

#include "serial.h"
#include "json.h"
#include "utils.h"
#include "config.h"

int setupSerial(const char *port, int baudrate) {
	int fd = -1;
	DBG("%s\n", port);
	fd = serialport_init(port, baudrate);
	if( fd==-1 ) {
		fprintf(stderr,"couldn't open port %s @ %d bauds\n", port, baudrate);
		return -1;
	}
	DBG("opened port %s @ %d bps\n",port, baudrate);

	serialport_flush(fd);
	return fd;
};

int readSerial(int fd, char *buf, int buf_max, int timeout) {
	char eolchar = '\n';
	int sr = serialport_read_until(fd, buf, eolchar, buf_max, timeout);
	DBG("DEBUG:\t%s", buf);

	return sr;
};

int arduinoEvent(char *buf, last_update_t *last, config_t *cfg, struct mosquitto *mosq) {
	time_t now = last->time;
	if(checkJSON_integer(buf,"code", 200)==0) {
		now = time(NULL);
	};
	if(strlen(buf)>0) {
		json_error_t error;
		json_t *root = json_loads(buf, 0, &error);

		void *id = json_object_iter_at(root, "id");
		if(id == NULL)
			return 1;
		json_t *json_device = json_object_iter_value(id);
		const char *device = json_string_value(json_device);

		const char *key;
		json_t *value;
		void *iter = json_object_iter(root);
		while(iter) {
			key = json_object_iter_key(iter);
			value = json_object_iter_value(iter);
			/* use key and value ... */
			char *topic = NULL;
			char *payload = NULL;
			asprintf(&topic, "%s/%s", device, key);
			if(json_is_integer(value)) {
				asprintf(&payload, "%lld", json_integer_value(value));
			} else if(json_is_real(value)) {
				asprintf(&payload, "%f", json_real_value(value));
			} else if(json_is_boolean(value)) {
				if(json_is_true(value))
					asprintf(&payload, "true");
				else
					asprintf(&payload, "false");

			} else {
				asprintf(&payload, "%s", json_string_value(value));
			}
			mosquitto_publish(mosq, NULL, topic, strlen(payload), payload, 0, true);
			DBG("%s -> %s\n", topic, payload);
			free(payload);
			free(topic);
			iter = json_object_iter_next(root, iter);
		}


	};
	bzero(buf,BUF_MAX);
	return 0;
}

void log_callback(struct mosquitto *mosq, void *userdata, int level, const char *str) {
	/* Pring all log messages regardless of level. */
#ifdef DEBUG_MQTT
	DBG("[MQTT LOG] %s\n", str);
#endif
}

void message_callback(struct mosquitto *mosq, void *userdata, const struct mosquitto_message *message) {
	DBG("%s -> %s\n", message->topic, message->payload);

	//serialport_write(arduino_fd, buf);

}

int main( int argc, char* argv[] ) {
	config_t cfg;
	struct mosquitto *mosq = NULL;

	config_init(&cfg);
	loadDefaults(&cfg);

	config_write(&cfg, stderr);

	if(parseArgs(argc, argv, &cfg)) {
		exit(1);
	}

	config_write(&cfg, stderr);

	port_t serial;
	loadSerial(&cfg, &serial);

	if(!strlen(serial.name)) {
		printf("You must specify the serial port\n");
		exit(2);
	}

	int arduino_fd = setupSerial(serial.name, serial.speed);
	if(arduino_fd < 0) {
		printf("Failed to setup serial port %s @ %d\n", serial.name, serial.speed);
		exit(2);
	}

	mosquitto_lib_init();
	mqttserver_t mqtt;
	loadMQTT(&cfg, &mqtt);

	char hostname[BUF_MAX];
	gethostname(hostname, BUF_MAX);
	char *client_id;
	asprintf(&client_id, "%s.%s", hostname, serial.name);
	mosq = mosquitto_new(client_id, true, NULL); //use port name as client id
	if(!mosq) {
		printf("Couldn't create a new mosquitto client instance\n");
		exit(3);
	}

	//TODO setup callbacks
	mosquitto_log_callback_set(mosq, log_callback);
    mosquitto_message_callback_set(mosq, message_callback);

	if(mosquitto_connect(mosq, mqtt.servername, mqtt.port, mqtt.keepalive)){
		printf("Unable to connect to %s:%d.\n", mqtt.servername, mqtt.port);
		exit(3);
	}

	//Subscribe command
	char *sub;
	asprintf(&sub, "%s/cmd", client_id);
	int r = mosquitto_subscribe(mosq, NULL, sub, 2);
	DBG("Subscribe %s = %d\n", sub, r);

	int mosq_fd = mosquitto_socket(mosq);

	fd_set active_fd_set, read_fd_set;
	/* Initialize the set of active sockets. */
	FD_ZERO (&active_fd_set);
	FD_SET (arduino_fd, &active_fd_set);
	FD_SET (mosq_fd, &active_fd_set);

	char buf[BUF_MAX];
	bzero(buf,BUF_MAX);

	last_update_t last;
	last.time = time(NULL);
	last.buf = malloc(BUF_MAX);
	bzero(last.buf,BUF_MAX);

	int nbytes;

	while(1) {
		/* Block until input arrives on one or more active sockets. */
		read_fd_set = active_fd_set;
		if (select (FD_SETSIZE, &read_fd_set, NULL, NULL, NULL) < 0) {
			fprintf(stderr, "Error in select\n");
			exit (EXIT_FAILURE);
		}

		/* Service all the sockets with input pending. */
		int i;
		for (i = 0; i < FD_SETSIZE; ++i)
			if (FD_ISSET (i, &read_fd_set)) {
				if(i == arduino_fd) {
					if(!readSerial(arduino_fd, buf, BUF_MAX, serial.timeout)) {
//						DBG("Read %d bytes\n", strlen(buf));
						serialport_flush(arduino_fd);
						arduinoEvent(buf, &last, &cfg, mosq);
					}
				} else if(i == mosq_fd) {
					mosquitto_loop_read(mosq, 1);
					mosquitto_loop_write(mosq, 1);
					mosquitto_loop_misc(mosq);
				} else {
					nbytes = read(i, buf, BUF_MAX);
					if (nbytes < 0) {
						/* Read error. */
						perror ("read");
						exit (EXIT_FAILURE);
					} else if (nbytes == 0) {
						/* End-of-file. */
						close(i);
						FD_CLR(i, &active_fd_set);
					} else {
						/* Data read. */
						fprintf (stderr, "Server: got message: `%s'\n", buf);
						if(!strcmp(buf, "exit")) {
							return EXIT_SUCCESS;
						}
						serialport_write(arduino_fd, buf);
					}

				}
			}
	};

	/* Cleanup */
	mosquitto_destroy(mosq);
	mosquitto_lib_cleanup();

	return EXIT_SUCCESS;
}