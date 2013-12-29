#!/usr/bin/python
import logging
import sys
import mosquitto
import urllib2
import re
from BeautifulSoup import BeautifulSoup

#Customize here
url = 'http://192.168.1.79/listdev.htm'

xively = dict(
feedid = "1873623042", #Xively FeedID
apikey = "g1KD3DJBOKoLMG0hZ0jCHWgMpefJdblml9TfXWTzgMXhQHvn", #Xively API Key
datastreams = "power, energy, energySpent"
)

#DON'T EDIT BELOW!!!
waitingFor = []

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
		lastupdate = soup.findAll('td')[21].contents[0]
	except Exception as e:
		logging.exception(e)
	return {'energy': energy,
			'power': power,
			'lastupdate': str(lastupdate)}

def setup_xively(mqttc, topic, xively_conf):
	for key in xively_conf.keys():
		r, mid = mqttc.publish(topic + "/xively/" + key, xively_conf[key], retain=True)
		if r != mosquitto.MOSQ_ERR_SUCCESS:
			logging.error("ERROR on setup_xively")
		else:
			waitingFor.append(mid)

def on_connect(mqttc, obj, rc):
	logging.info("Connected")
	if "setup" in sys.argv:
		setup_xively(mqttc, "imeter", xively)

	r, mid = mqttc.subscribe("imeter/energy")
	waitingFor.append(mid)

def on_subscribe(mqttc, userdata, mid, granted_qos):
	logging.debug("on_subscribe " + str(mid) + " in " + str(waitingFor))

	if mid in waitingFor:
		waitingFor.remove(mid)

def on_message(mqttc, userdata, msg):
	logging.debug(msg.topic + " -> " + msg.payload)

	userdata['energySpent'] = int(userdata['energy']) - int(msg.payload)

def publish(mqttc, topic, data):
	logging.info("Publish data")
	for key in data.keys():
		r, mid = mqttc.publish(topic + "/" + key, data[key], retain=False)
		if r != mosquitto.MOSQ_ERR_SUCCESS:
			logging.error("ERROR on publish")
		else:
			waitingFor.append(mid)

def on_publish(mqttc, obj, mid):
	logging.debug("on_publish " + str(mid) + " in " + str(waitingFor))
	if mid in waitingFor:
		waitingFor.remove(mid)

def on_log(mqttc, obj, level, string):
	logging.debug(string)

def main():
	logging.basicConfig(format='[%(asctime)s] %(levelname)s - %(message)s', datefmt='%d/%m/%Y %I:%M:%S %p', level=logging.INFO)
	logging.info("iMeter - v1")

	#get data
	data = parseWebpage()

	mqttc = mosquitto.Mosquitto("iMeter", True, data)
	mqttc.on_message = on_message
	mqttc.on_connect = on_connect
	mqttc.on_publish = on_publish
	mqttc.on_subscribe = on_subscribe
	mqttc.connect("192.168.1.10", 1883, 60)

	rc = mqttc.loop()
	while rc == 0 and len(waitingFor) > 0:
		logging.debug(waitingFor)
		rc = mqttc.loop()
	rc = mqttc.loop()
	publish(mqttc, "imeter", data)

	return 0

if __name__ == "__main__":
	sys.exit(main())
