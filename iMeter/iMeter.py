#!/usr/bin/python
import logging
import datetime 
import sys
import mosquitto
import urllib2
import re
from BeautifulSoup import BeautifulSoup

#Customize here
url = 'http://192.168.1.79/listdev.htm'
broker = 'home.diogogomes.com'
port = 1883

#DON'T EDIT BELOW!!!
published = False

def parseWebpage():

	usock = urllib2.urlopen(url)
	html = usock.read()
	usock.close()

	try:
		soup = BeautifulSoup(html)
		#this is fine tuned :)
		scrap = str(soup.findAll('td')[15])
		non_decimal = re.compile(r'[^\d\n.]+')
		scrap = non_decimal.sub('', scrap)
		data = scrap.split('\n')
		energy = int(data[1])
		power = int(data[2])
		lastupdate = "\"" + soup.findAll('td')[21].contents[0] +"\""
	except Exception as e:
		logging.exception(e)
	return {'energy': energy,
			'power': power,
			'lastupdate': str(lastupdate)}

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
	raw = "{\"id\": \"imeter\""
	for key in data.keys():
		r, mid = mqttc.publish(topic + "/" + key, data[key], retain=True)
		logging.debug(topic+"/"+ str(key) + " -> " + str(data[key]))
		raw = raw + ", \"" + str(key) + "\": " + str(data[key])
		if r != mosquitto.MOSQ_ERR_SUCCESS:
			logging.error("ERROR on publish")
	raw = raw + "}"
	mqttc.publish(topic + "/raw", raw, retain=False)

def on_log(mqttc, obj, level, string):
	pass
#	logging.debug(string)

def main():
	logging.basicConfig(format='[%(asctime)s] %(levelname)s - %(message)s', datefmt='%d/%m/%Y %I:%M:%S %p', level=logging.DEBUG)
	logging.info("iMeter - v2")

	#get data
	data = parseWebpage()

	mqttc = mosquitto.Mosquitto("iMeter", True, data)
	mqttc.on_log = on_log
	mqttc.on_message = on_message
	mqttc.on_connect = on_connect
	mqttc.connect(broker, port, 60)

	mqttc.loop_forever()

	return 0

if __name__ == "__main__":
	sys.exit(main())
