#include "serialconfig.h"

int parseSerialArgs(int argc, char *argv[], config_t *c) {
	int ch;
    optind = 0;

	/* options descriptor */
	const struct option longopts[] = {
		{ "port",      required_argument,            0,           'p' },
		{ "port-speed",   required_argument,      0,           1 },
		{ NULL,         0,                      NULL,           0 }
	};
	const char *usage = "usage: %s -c|--conf configuration-file -p|--port devname [--port-speed 115200] [--server localhost] [--server-port 1883] [--save]\n";

	int option_index = 0;

	config_setting_t *setting;
	config_setting_t *root = config_root_setting(c);
	config_setting_t *serial;

    while ((ch = getopt_long(argc, argv, "p:", longopts, &option_index)) != -1) {
		DBG("ch = %d	option_index = %d	optarg = %s\n", ch, option_index, optarg);
		switch (ch) {
			case 'p':
				serial = config_setting_get_member(root, CONFIG_SERIAL);
				setting = config_setting_get_member(serial, CONFIG_SERIAL_DEVNAME);
				if(config_setting_is_scalar(setting) == CONFIG_FALSE)
					setting = config_setting_add(serial, CONFIG_SERIAL_DEVNAME, CONFIG_TYPE_STRING);
				DBG("Setting port %s\n", optarg);
				config_setting_set_string(setting, optarg);
				break;
			case 1:
				switch(option_index) {
					case 0:
						serial = config_setting_get_member(root, CONFIG_SERIAL);
						setting = config_setting_get_member(serial, CONFIG_SERIAL_SPEED);
						if(config_setting_is_scalar(setting) == CONFIG_FALSE)
							setting = config_setting_add(serial, CONFIG_SERIAL_SPEED, CONFIG_TYPE_INT);
						DBG("Setting port speed %s\n", optarg);
						config_setting_set_int(setting, atoi(optarg));
						break;
				}
				break;
			case '?':
                break;
			default:
				fprintf(stdout,usage, argv[0]);
				return 1;
		}
	}
	return 0;
}

int loadSerialDefaults(config_t *c) {
	config_setting_t *root = config_root_setting(c);

	// Serial
	config_setting_t *serial = config_setting_get_member(root, CONFIG_SERIAL);
	if(serial == NULL)
		serial = config_setting_add(root, CONFIG_SERIAL, CONFIG_TYPE_GROUP);

	config_setting_t *devname = config_setting_get_member(serial, CONFIG_SERIAL_DEVNAME);
	if(devname == NULL) {
		devname = config_setting_add(serial, CONFIG_SERIAL_DEVNAME, CONFIG_TYPE_STRING);
	}
	config_setting_t *speed = config_setting_get_member(serial, CONFIG_SERIAL_SPEED);
	if(speed == NULL) {
		speed = config_setting_add(serial, CONFIG_SERIAL_SPEED, CONFIG_TYPE_INT);
		config_setting_set_int(speed, DEFAULT_SERIAL_SPEED);
	}
	config_setting_t *timeout = config_setting_get_member(serial, CONFIG_SERIAL_TIMEOUT);
	if(timeout == NULL) {
		timeout = config_setting_add(serial, CONFIG_SERIAL_TIMEOUT, CONFIG_TYPE_INT);
		config_setting_set_int(timeout, DEFAULT_SERIAL_TIMEOUT);
	}

	return 0;
}

int loadSerial(config_t *c, port_t *port) {
	config_setting_t *root = config_root_setting(c);
	config_setting_t *serial = config_setting_get_member(root, CONFIG_SERIAL);

	port->name = NULL;
	if(!(config_setting_lookup_string(serial, CONFIG_SERIAL_DEVNAME, &port->name) == CONFIG_TRUE
                && config_setting_lookup_int(serial, CONFIG_SERIAL_SPEED, &port->speed) == CONFIG_TRUE
                && config_setting_lookup_int(serial, CONFIG_SERIAL_TIMEOUT, &port->timeout) == CONFIG_TRUE
            ))
		return 1;

	return 0;
}
