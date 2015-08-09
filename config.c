#include "config.h"

int parseArgs(int argc, char *argv[], config_t *c) {
	int ch;
	char *conffile = NULL;

	/* options descriptor */
	const struct option longopts[] = {
		{ "conf",      required_argument,            0,           'c' },
		{ "server",  required_argument,           0,     's' },
		{ "server-port",  required_argument,      0,     1 },
		{ "save",  no_argument,      0,     1 },
		{ NULL,         0,                      NULL,           0 }
	};
	const char *usage = "usage: %s -c|--conf configuration-file [--server localhost] [--server-port 1883] [--save]\n";

	int option_index = 0;

	config_setting_t *setting;
	config_setting_t *root = config_root_setting(c);
	config_setting_t *mqtt;

	int persist = 0;

	while ((ch = getopt_long(argc, argv, "c:s:", longopts, &option_index)) != -1) {
		DBG("ch = %d	option_index = %d	optarg = %s\n", ch, option_index, optarg);
		switch (ch) {
			case 'c':
				//TODO make config file argument non required
				DBG("Reading from %s\n", optarg);
				config_read_file(c, optarg);
				conffile = strdup(optarg);
				break;
			case 's':
				mqtt = config_setting_get_member(root, CONFIG_MQTT);
				setting = config_setting_get_member(mqtt, CONFIG_MQTT_SERVER);
				if(config_setting_is_scalar(setting) == CONFIG_FALSE)
					 setting = config_setting_add(mqtt, CONFIG_MQTT_SERVER, CONFIG_TYPE_STRING);
				DBG("Setting mqttserver %s\n", optarg);
				config_setting_set_string(setting, optarg);
				break;
			case 1:
				switch(option_index) {
					case 2:
						mqtt = config_setting_get_member(root, CONFIG_MQTT);
						setting = config_setting_get_member(mqtt, CONFIG_MQTT_PORT);
						if(config_setting_is_scalar(setting) == CONFIG_FALSE)
							setting = config_setting_add(mqtt, CONFIG_MQTT_PORT, CONFIG_TYPE_INT);
						DBG("Setting mqttserver port %s\n", optarg);
						config_setting_set_int(setting, atoi(optarg));
						break;
					case 3:
						if(conffile) {
							persist = 1;
						}
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
	if(persist) {
		DBG("Persist to file %s\n", conffile);
		config_write_file(c, conffile);
	}

	free(conffile);
	return 0;
}

int loadDefaults(config_t *c) {
	config_setting_t *root = config_root_setting(c);

	// MQTT
	config_setting_t *mqtt = config_setting_get_member(root, CONFIG_MQTT);
	if(mqtt == NULL)
		mqtt = config_setting_add(root, CONFIG_MQTT, CONFIG_TYPE_GROUP);

	config_setting_t *server = config_setting_get_member(mqtt, CONFIG_MQTT_SERVER);
	if(server == NULL) {
		server = config_setting_add(mqtt, CONFIG_MQTT_SERVER, CONFIG_TYPE_STRING);
		config_setting_set_string(server, DEFAULT_MQTT_SERVER);
	}
	config_setting_t *server_port = config_setting_get_member(mqtt, CONFIG_MQTT_PORT);
	if(server_port == NULL) {
		server_port = config_setting_add(mqtt, CONFIG_MQTT_PORT, CONFIG_TYPE_INT);
		config_setting_set_int(server_port, DEFAULT_MQTT_SERVER_PORT);
	}
	config_setting_t *server_keep_alive = config_setting_get_member(mqtt, CONFIG_MQTT_KEEP_ALIVE);
	if(server_keep_alive == NULL) {
		server_keep_alive = config_setting_add(mqtt, CONFIG_MQTT_KEEP_ALIVE, CONFIG_TYPE_INT);
		config_setting_set_int(server_keep_alive, DEFAULT_MQTT_KEEP_ALIVE);
	}

	return 0;
}

int loadMQTT(config_t *c, mqttserver_t *mqtt) {
	config_setting_t *root = config_root_setting(c);
	config_setting_t *serial = config_setting_get_member(root, CONFIG_MQTT);

	if(!(config_setting_lookup_string(serial, CONFIG_MQTT_SERVER, &mqtt->servername)
                && config_setting_lookup_int(serial, CONFIG_MQTT_PORT, &mqtt->port)
                && config_setting_lookup_int(serial, CONFIG_MQTT_KEEP_ALIVE, &mqtt->keepalive)
            ))
		return 1;

	return 0;
}
