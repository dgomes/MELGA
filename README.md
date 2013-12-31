MELGA - MQTT-SN basEd LightweiGht home Automation
=================================================

Approach:
 * To monitor/control various aspects in the house such as: energy consumption, internet usage, greenhouse monitoring (temp, hum, light), power sockets, remote controls
 * No single daemon, several small programs:
   - complex parsers (Web Scrapper, SNMP) coded in Python and run through a CRON job
   - Lightweitght persistent daemons coded in C for housekeeping tasks (CRON, xively)
 * All IPC is handled using a MQTT-SN broker (http://mosquitto.org)
 * Persistent Storage is outsourced to Xively (http://xively.com)

Modules:
 * Power Consumption - iMeter (web scrapper)
 * Internet Monitoring (SNMP)
 * Greenhouse (arduino on a serial port)
 * Power Sockets (arduino on a serial port)

Dependencies
------------

 * libxively (https://github.com/xively/libxively)
 * libmosquitto (http://mosquitto.org/download/)
 * libconfig
