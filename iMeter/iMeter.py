#!/usr/bin/python

import sys
import mosquitto
import urllib2
import re
from BeautifulSoup import BeautifulSoup

#Customize here
url = 'http://192.168.1.79/listdev.htm'
feedid = "1873623042" #Xively FeedID
apikey = "g1KD3DJBOKoLMG0hZ0jCHWgMpefJdblml9TfXWTzgMXhQHvn" #Xively API Key

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
		print e
	return energy, power, str(lastupdate)

def setup(mqttc):
	#Xively
	r, mid = mqttc.publish("imeter/xively/feedid", feedid, retain=True)
	if r != mosquitto.MOSQ_ERR_SUCCESS:
		print "ERROR"
	else:
		waitingFor.append(mid)

	r, mid = mqttc.publish("imeter/xively/apikey", apikey, retain=True)
	if r != mosquitto.MOSQ_ERR_SUCCESS:
		print "ERROR"
	else:
		waitingFor.append(mid)

	r, mid = mqttc.publish("imeter/xively/datastreams", "power, energy, energySpent", retain=True)
	if r != mosquitto.MOSQ_ERR_SUCCESS:
		print "ERROR"
	else:
		waitingFor.append(mid)

def on_connect(mqttc, obj, rc):
	print "Connected"

	#required for energySpent
	r, mid = mqttc.subscribe("imeter/energy")
	if r != mosquitto.MOSQ_ERR_SUCCESS:
		print "ERROR"
	else:
		waitingFor.append(mid)
	#we now wait for energy to be used to publish
	waitingFor.append("pub")

	if "setup" in sys.argv:
		setup(mqttc)


def on_subscribe(mosq, userdata, mid, granted_qos):
	#print "on_subscribe " + str(mid) + " in " + str(waitingFor)

	if mid in waitingFor:
		waitingFor.remove(mid)

def on_message(mosq, obj, msg):
	energy, power, lastupdate = parseWebpage()

	print msg.topic

	#EnergySpent
	energySpent = -1
	if msg.topic == "imeter/energy" and len(msg.payload):
		energySpent = int(energy) - int(msg.payload)
		r, mid = mosq.publish("imeter/energySpent", energySpent, retain=True)
		if r != mosquitto.MOSQ_ERR_SUCCESS:
			print "ERROR"
		else:
			waitingFor.append(mid)
			#we don't need energy anymore
			mosq.unsubscribe("imeter/energy")

	#Energy
	r, mid = mosq.publish("imeter/energy", energy, retain=True)
	if r != mosquitto.MOSQ_ERR_SUCCESS:
		print "ERROR"
	else:
		waitingFor.append(mid)

	#LastUpdate
	r, mid = mosq.publish("imeter/lastUpdate", lastupdate, retain=True)
	if r != mosquitto.MOSQ_ERR_SUCCESS:
		print "ERROR"
	else:
		waitingFor.append(mid)

	#Power
	r, mid = mosq.publish("imeter/power", power, retain=True)
	if r != mosquitto.MOSQ_ERR_SUCCESS:
		print "ERROR"
	else:
		waitingFor.append(mid)

	print "Last Update:" + str(lastupdate) +", Energy:" + str(energy) + ", Power:" + str(power) + ", EnergySpent:" + str(energySpent)

	waitingFor.remove("pub")

def on_publish(mosq, obj, mid):
	#print "on_publish " + str(mid) + " in " + str(waitingFor)
	if mid in waitingFor:
		waitingFor.remove(mid)

def on_log(mosq, obj, level, string):
	print(string)

def main():
	print "iMeter - v0"

	mqttc = mosquitto.Mosquitto()
	mqttc.on_message = on_message
	mqttc.on_connect = on_connect
	mqttc.on_publish = on_publish
	mqttc.on_subscribe = on_subscribe
	mqttc.connect("192.168.1.10", 1883, 60)


	rc = mqttc.loop()
	timeout = 0
	while rc == 0 and len(waitingFor) > 0:
		rc = mqttc.loop()
		#print waitingFor
		#this timeout enables the detection of a deadlock in case we are publishing energy for the 1st time
		timeout+=1
		if timeout > 5:
			mqttc.publish("imeter/energy")

	return 0

if __name__ == "__main__":
	sys.exit(main())
