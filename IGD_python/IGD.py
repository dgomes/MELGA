#!/usr/bin/python
import logging
import sys
import mosquitto
import urllib2
import re
from BeautifulSoup import BeautifulSoup

#Customize here
url = 'http://192.168.1.79/listdev.htm'
broker = 'localhost'
port = 1883

#DON'T EDIT BELOW!!!
published = False

def readIGD():
	logging.info("Not implemented")
	sys.exit(1)


def on_connect(mqttc, obj, rc):
	logging.info("Connected")

	mqttc.subscribe("imeter/energy")

def on_message(mqttc, userdata, msg):
	global published
	logging.debug("on_message: " + msg.topic + " -> " + msg.payload)

	userdata['energySpent'] = int(userdata['energy']) - int(msg.payload)
	
	if published:
		mqttc.disconnect()
	else:
		publish(mqttc, "imeter", userdata)
		published = True

def publish(mqttc, topic, data):
	logging.info("Publish data")
	for key in data.keys():
		r, mid = mqttc.publish(topic + "/" + key, data[key], retain=True)
		logging.debug(topic+"/"+ str(key) + " -> " + str(data[key]))
		if r != mosquitto.MOSQ_ERR_SUCCESS:
			logging.error("ERROR on publish")

def on_log(mqttc, obj, level, string):
	pass
#	logging.debug(string)

def main():
	logging.basicConfig(format='[%(asctime)s] %(levelname)s - %(message)s', datefmt='%d/%m/%Y %I:%M:%S %p', level=logging.DEBUG)
	logging.info("IGD - v1")

	#get data
	data = readIGD()

	mqttc = mosquitto.Mosquitto("iMeter", True, data)
	mqttc.on_log = on_log
	mqttc.on_message = on_message
	mqttc.on_connect = on_connect
	mqttc.connect(broker, port, 60)

	mqttc.loop_forever()	

	return 0

if __name__ == "__main__":
	sys.exit(main())