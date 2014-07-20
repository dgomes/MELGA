#include "config.h"

int parseArgs(int argc, char *argv[], config_t *c) {
	int ch;

	/* options descriptor */
	const struct option longopts[] = {
		{ "conf",      required_argument,            0,           1 },
		{ "port",      required_argument,            0,           1 },
		{ "port-speed",   required_argument,      0,           1 },
		{ "server",  required_argument,           0,     1 },
		{ "server-port",  required_argument,      0,     1 },
		{ NULL,         0,                      NULL,           0 }
	};
	const char *usage = "usage: %s --conf configuration-file --port devname [--port-speed 115200] [--server localhost] [--server-port 1883]\n";

	int option_index = 0;

	while ((ch = getopt_long(argc, argv, "d", longopts, &option_index)) != -1) {
		fprintf(stderr, "ch = %d	option_index = %d	optarg = %s\n", ch, option_index, optarg);
		switch (ch) {
			case 'd':
				DBG("Debug ON\n");
				break;
			case 1:
				switch(option_index) {
					case 0:
						c->conffile = strdup(optarg);
						break;
					case 1:
						c->port.name = strdup(optarg);
						break;
					case 2:
						c->port.speed = atoi(optarg);
						break;
					case 3:
						c->remote.servername = strdup(optarg);
						break;
					case 4:
						c->remote.port = atoi(optarg);
						break;
				}
				break;
			case '?':
			default:
				fprintf(stdout,usage, argv[0]);
				return 1;
		}
	}
	if( c->conffile == NULL) {
		fprintf(stdout,usage, argv[0]);
		return 1;
	}
	return 0;
}

int loadDefaults(config_t *c) {
	c->port.speed = 115200;
	c->port.timeout = 5000;
	c->remote.servername = strdup("localhost");
	c->remote.port = 1883;
	c->remote.keepalive = 1800; //seconds
	c->port.name = NULL;
	c->conffile = NULL;
	return 0;
}

char *dumpConfig(config_t *c) {
	//TODO dump config_t to a json string
	char *dump;
	asprintf(&dump, "{ \"port\": {\"name\": \"%s\", \"speed\": %d, \"timeout\": %d}, \"remote\": {\"servername\": \"%s\", \"port\": %d, \"keepalive\": %d } }", c->port.name, c->port.speed, c->port.timeout, c->remote.servername, c->remote.port, c->remote.keepalive);

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
	asprintf(&c->port.name, "%s", json_string_value(name));
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
