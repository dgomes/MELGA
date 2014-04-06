#include "config.h"

int parseArgs(int argc, char *argv[], config_t *c) {
	int ch;
	int port, port_speed, server, server_port, dev;

	/* options descriptor */
	const struct option longopts[] = {
		{ "device",      required_argument,            &dev,           1 },
		{ "port",      required_argument,            &port,           1 },
		{ "port-speed",   required_argument,      &port_speed,           1 },
		{ "server",  required_argument,            &server,     1 },
		{ "server-port",  required_argument,            &server_port,     1 },
		{ NULL,         0,                      NULL,           0 }
	};
	const char *usage = "usage: %s --port devname [--port-speed 115200] [--server localhost] [--server-port 1883]\n";

	while ((ch = getopt_long(argc, argv, "d", longopts, NULL)) != -1) {
		switch (ch) {
			case 'd':
				fprintf(stderr, "Debug ON\n");
				break;
			case 0:
				if (port) {
					c->port.name = strdup(optarg);
					port = 0;
				} else if (port_speed) {
					c->port.speed = atoi(optarg);
					port_speed = 0;
				} else if (dev) {
					c->device = strdup(optarg);
					dev = 0;
				} else if (server) {
					c->remote.servername = strdup(optarg);
					server = 0;
				}
				break;
			case '?':
			default:
				printf(usage, argv[0]);
				return 1;
		}
	}
	return 0;
}

int loadDefaults(config_t *c) {
	c->port.speed = 115200;
	c->port.timeout = 5000;
	c->remote.servername = strdup("localhost");
	c->remote.port = 1883;
	c->remote.keepalive = 1800; //seconds
	c->device = NULL;
	c->port.name = NULL;
	return 0;
}

char *dumpConfig(config_t *c) {
	//TODO dump config_t to a json string
	char *dump = malloc(128);
	asprintf(&dump, "{ 'device': '%s', 'port': {'name': '%s', 'speed': %d, 'timeout': %d}, 'remote': {'servername': '%s', 'port': %d, 'keepalive': %d } }", c->device, c->port.name, c->port.speed, c->port.timeout, c->remote.servername, c->remote.port, c->remote.keepalive);

	return dump;
}

int readConfig(const char *filename, config_t *c) {
	if(!strlen(filename)) return -1;

	json_error_t error;
	json_t *root = json_load_file(filename, 0, &error);

	if(!root) {
		fprintf(stderr, "error: on line %d: %s\n", error.line, error.text);
		return 1;
	}

	/* Port Configuration */
	json_t *port = json_object_get(root, "port");
	if(!json_is_object(port)) {
		fprintf(stderr, "error: port is not an object\n");
		return 1;
	}

	json_t *name = json_object_get(port, "name");
	if(!json_is_string(name)) {
		fprintf(stderr, "error: port name missing\n");
		return 1;
	}
	c->port.name = malloc(strlen(json_string_value(name)));
	snprintf(c->port.name, strlen(json_string_value(name)),"%s", json_string_value(name));
	json_t *speed = json_object_get(port, "speed");
	if(!json_is_integer(speed)) {
		fprintf(stderr, "error: port speed missing\n");
		return 1;
	}
	c->port.speed = json_integer_value(speed);
	json_t *timeout = json_object_get(port, "timeout");
	if(!json_is_integer(timeout)) {
		fprintf(stderr, "error: port timeout missing\n");
		return 1;
	}
	c->port.timeout = json_integer_value(timeout);

	/* Remote Configuration */
	json_t *remote = json_object_get(root, "remote");
	if(!json_is_object(remote)) {
		fprintf(stderr, "error: remote is not an object\n");
		return 1;
	}
	json_t *remote_port = json_object_get(remote, "port");
	if(!json_is_integer(remote_port)) {
		fprintf(stderr, "error: remote port missing\n");
		return 1;
	}
	c->remote.port = json_integer_value(remote_port);

	return 0;
}
