MELGA - MQTT-SN basEd LightweiGht home Automation
=================================================

OK I know the acronym is not very good :) but I really like the name of the project :) 

Approach:
 * To monitor/control various aspects in the house such as: energy consumption, internet usage, greenhouse monitoring (temp, hum, light), power sockets, remote controls
 * No single daemon, several small programs:
   - complex parsers (Web Scrapper, SNMP) coded in Python and run through a CRON job
   - Lightweitght persistent daemons coded in C for housekeeping tasks (upnp-igd, xively)
 * All IPC is handled using a MQTT-SN broker (http://mosquitto.org)
 * Persistent Storage is outsourced to Xively (http://xively.com)

Modules:
 * Power Consumption - iMeter (web scrapper)
 * Internet Monitoring - UPnP IGD (IGD)
 * Greenhouse / Weather / Power Sockets (arduino on a serial port)
 * Xively - Uploading information to http://xively.com

Triggers:
 Almost all triggers are handled by mqttwarn, even if they call external scripts in "actions"
 * turn lights
 * notify door opening
 * notify killed battery

Dependencies
------------

 * mqttwarn (https://github.com/jpmens/mqttwarn/)
 * Supervisor (http://supervisord.org)
 * libxively (https://github.com/xively/libxively)
 * libmosquitto (http://mosquitto.org/download/)
 * libconfig
 * libjansson
 * pkg-config

Install
-------

 * Install mqttwarn (e.g. /opt/mqttwarn)
 * Install Supervisor
 * use the example files in the "extra" directory 
