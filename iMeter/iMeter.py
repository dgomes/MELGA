import sys
import mosquitto
import urllib2
import re
from BeautifulSoup import BeautifulSoup

waitingFor = []

def parseWebpage():
	url = 'http://192.168.1.79/listdev.htm'

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

def on_connect(mosq, obj, rc):
	r, mid = mosq.subscribe("imeter/energy")
	if r != mosquitto.MOSQ_ERR_SUCCESS:
		print "ERROR"
	else:
		waitingFor.append(mid)

def on_subscribe(mosq, userdata, mid, granted_qos):
	if mid in waitingFor:
		waitingFor.remove(mid)
	waitingFor.append("energySpent")

def on_message(mosq, obj, msg):
	energy, power, lastupdate = parseWebpage()

	#EnergySpent
	if msg.topic == "imeter/energy":
		energySpent = int(energy) - int(msg.payload)
	else:
		energySpent = 0
	waitingFor.remove("energySpent")

	#LastUpdate
	r, mid = mosq.publish("imeter/lastUpdate", lastupdate, retain=True)
	if r != mosquitto.MOSQ_ERR_SUCCESS:
		print "ERROR"
	else:
		waitingFor.append(mid)

	#Energy
	r, mid = mosq.publish("imeter/energy", energy, retain=True)
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


def on_publish(mosq, obj, mid):
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

	#Xively
	r, mid = mosq.publish("imeter/xively/id", "123456789", retain=True)
	if r != mosquitto.MOSQ_ERR_SUCCESS:
		print "ERROR"
	else:
		waitingFor.append(mid)

	#Xively
	r, mid = mosq.publish("imeter/xively/datastreams", "power, energy, energySpent", retain=True)
	if r != mosquitto.MOSQ_ERR_SUCCESS:
		print "ERROR"
	else:
		waitingFor.append(mid)


	rc = mqttc.loop()
	while rc == 0 and len(waitingFor) > 0:
		rc = mqttc.loop()

	return 0

if __name__ == "__main__":
	sys.exit(main())
