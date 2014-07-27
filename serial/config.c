#include "config.h"

int parseArgs(int argc, char *argv[], config_t *c) {
	int ch;
	char *conffile = NULL;

	/* options descriptor */
	const struct option longopts[] = {
		{ "conf",      required_argument,            0,           1 },
		{ "port",      required_argument,            0,           1 },
		{ "port-speed",   required_argument,      0,           1 },
		{ "server",  required_argument,           0,     1 },
		{ "server_port",  required_argument,      0,     1 },
		{ NULL,         0,                      NULL,           0 }
	};
	const char *usage = "usage: %s --conf configuration-file --port devname [--port-speed 115200] [--server localhost] [--server_port 1883] -p\n";

	int option_index = 0;

	config_setting_t *setting;
	config_setting_t *root = config_root_setting(c);
	config_setting_t *serial = config_setting_get_member(root, "serial");
	config_setting_t *mqtt = config_setting_get_member(root, "mqtt");

	while ((ch = getopt_long(argc, argv, "p", longopts, &option_index)) != -1) {
		//DBG("ch = %d	option_index = %d	optarg = %s\n", ch, option_index, optarg);
		switch (ch) {
			case 'p':
				DBG("Persist to file\n");
				if(conffile)
					config_write_file(c, conffile);
				break;
			case 1:
				switch(option_index) {
					case 0:
						DBG("Reading from %s\n", optarg);
						config_read_file(c, optarg);
						conffile = strdup(optarg);
						break;
					case 1:
						setting = config_setting_get_member(serial, "port");
						if(setting != NULL)
							config_setting_set_int(setting, atoi(optarg));
						break;
					case 2:
						setting = config_setting_get_member(serial, "port-speed");
						if(setting != NULL)
							config_setting_set_int(setting, atoi(optarg));
						break;
					case 3:
						setting = config_setting_get_member(mqtt, "server");
						if(setting != NULL)
							config_setting_set_string(setting, optarg);
						break;
					case 4:
						setting = config_setting_get_member(mqtt, "server_port");
						if(setting != NULL)
							config_setting_set_int(setting, atoi(optarg));
						break;
				}
				break;
			case '?':
			default:
				fprintf(stdout,usage, argv[0]);
				return 1;
		}
	}
	if(conffile == NULL) {
		fprintf(stdout,usage, argv[0]);
		return 1;
	}
	return 0;
}

int loadDefaults(config_t *c) {
	config_setting_t *root = config_root_setting(c);

	// Serial
	config_setting_t *serial = config_setting_get_member(root, "serial");
	if(serial == NULL)
		serial = config_setting_add(root, "serial", CONFIG_TYPE_GROUP);

	config_setting_t *devname = config_setting_get_member(serial, "devname");
	if(devname == NULL) {
		devname = config_setting_add(serial, "devname", CONFIG_TYPE_STRING);
	}
	config_setting_t *speed = config_setting_get_member(serial, "speed");
	if(speed == NULL) {
		speed = config_setting_add(serial, "speed", CONFIG_TYPE_INT);
		config_setting_set_int(speed, 115200);
	}
	config_setting_t *timeout = config_setting_get_member(serial, "timeout");
	if(timeout == NULL) {
		timeout = config_setting_add(serial, "timeout", CONFIG_TYPE_INT);
		config_setting_set_int(timeout, 30000);
	}


	// MQTT
	config_setting_t *mqtt = config_setting_get_member(root, "mqtt");
	if(mqtt == NULL)
		mqtt = config_setting_add(root, "mqtt", CONFIG_TYPE_GROUP);

	config_setting_t *server = config_setting_get_member(mqtt, "server");
	if(server == NULL) {
		server = config_setting_add(mqtt, "server", CONFIG_TYPE_STRING);
		config_setting_set_string(server, "localhost");
	}
	config_setting_t *server_port = config_setting_get_member(mqtt, "server_port");
	if(server_port == NULL) {
		server_port = config_setting_add(mqtt, "server_port", CONFIG_TYPE_INT);
		config_setting_set_int(server_port, 1883);
	}
	config_setting_t *server_keep_alive = config_setting_get_member(mqtt, "keep_alive");
	if(server_keep_alive == NULL) {
		server_keep_alive = config_setting_add(mqtt, "keep_alive", CONFIG_TYPE_INT);
		config_setting_set_int(server_keep_alive, 1800);
	}

	return 0;
}

int loadSerial(config_t *c, port_t *port) {
	config_setting_t *root = config_root_setting(c);
	config_setting_t *serial = config_setting_get_member(root, "serial");

	port->name = NULL;
	if(!(config_setting_lookup_string(serial, "devname", &port->name)
                && config_setting_lookup_int(serial, "speed", &port->speed)
                && config_setting_lookup_int(serial, "timeout", &port->timeout)
            ))
		return 1;

	DBG("loadSerial\n");
	return 0;
}
int loadMQTT(config_t *c, mqttserver_t *mqtt) {
	config_setting_t *root = config_root_setting(c);
	config_setting_t *serial = config_setting_get_member(root, "mqtt");

	if(!(config_setting_lookup_string(serial, "server", &mqtt->servername)
                && config_setting_lookup_int(serial, "server_port", &mqtt->port)
                && config_setting_lookup_int(serial, "keep_alive", &mqtt->keepalive)
            ))
		return 1;

	return 0;
}
