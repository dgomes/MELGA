#!/usr/bin/python
import logging
import sys
import mosquitto
from pysnmp.entity.rfc3413.oneliner import cmdgen
from datetime import datetime
import calendar
#Customize here

#DON'T EDIT BELOW!!!
waitingFor = []

def getSNMP():

	cmdGen = cmdgen.CommandGenerator()

	errorIndication, errorStatus, errorIndex, varBinds = cmdGen.getCmd(
		cmdgen.CommunityData('public'),
		cmdgen.UdpTransportTarget(('192.168.1.72', 161)),
		cmdgen.MibVariable('AIRPORT-BASESTATION-3-MIB', 'wirelessNumber',0),
		cmdgen.MibVariable('RFC1213-MIB', 'ifInOctets',4),
		cmdgen.MibVariable('RFC1213-MIB', 'ifOutOctets',4),
		lookupNames=True, lookupValues=True
	)

	now = calendar.timegm(datetime.utcnow().utctimetuple())
	#	# Check for errors and print out results
	data = dict(update=now)
	if errorIndication:
		print(errorIndication)
	elif errorStatus:
		print(errorStatus)
	else:
		for name, val in varBinds:
			print('%s = %s' % (name.prettyPrint(), val.prettyPrint()))
			if name.prettyPrint() == 'AIRPORT-BASESTATION-3-MIB::wirelessNumber."0"':
				data["numberWirelessClients"] = int(val)
			else:
				data[str(name.prettyPrint())] = int(val)
	return data

def on_connect(mqttc, obj, rc):
	logging.info("Connected")

	r, mid = mqttc.subscribe('airport/update')
	waitingFor.append(mid)

def on_subscribe(mqttc, userdata, mid, granted_qos):
	logging.debug("on_subscribe " + str(mid) + " in " + str(waitingFor))

	if mid in waitingFor:
		waitingFor.remove(mid)
		waitingFor.append('loop')	#give it 1 loop to get a message

def on_message(mqttc, userdata, msg):
	logging.debug(msg.topic + " -> " + msg.payload)

	if "update" in msg.topic:
		userdata['lastUpdate'] = int(msg.payload)
		#if we have lastUpdate we can't calculate Rate's, so lets subscribe to last event
		r, mid = mqttc.subscribe('airport/RFC1213-MIB::ifOutOctets."4"')
		waitingFor.append(mid)
		r, mid = mqttc.subscribe('airport/RFC1213-MIB::ifInOctets."4"')
		waitingFor.append(mid)
	elif "Octets" in  str(msg.topic[(len('airport')+1):]):
		if "In" in str(msg.topic[(len('airport')+1):]):
			top = "downloadRate"
		elif "Out" in str(msg.topic[(len('airport')+1):]):
			top = "uploadRate"
		userdata[top] = (userdata[msg.topic[(len('airport')+1):]] - int(msg.payload)) * 8 / (1024 * (int(userdata['update']) - int(userdata['lastUpdate'])))

def publish(mqttc, topic, data):
	logging.info("Publish data")
	for key in data.keys():
		logging.debug(topic + "/" + key + " = " + str(data[key]))
		r, mid = mqttc.publish(topic + "/" + key, data[key], retain=True)
		if r != mosquitto.MOSQ_ERR_SUCCESS:
			logging.error("ERROR on publish")

def on_publish(mqttc, obj, mid):
	logging.debug("on_publish " + str(mid) + " in " + str(waitingFor))
	if mid in waitingFor:
		waitingFor.remove(mid)
def on_log(mqttc, obj, level, string):
	logging.debug(string)

def main():
	logging.basicConfig(format='[%(asctime)s] %(levelname)s - %(message)s', datefmt='%d/%m/%Y %I:%M:%S %p', level=logging.DEBUG)
	logging.info("Airport - v1")

	#get data
	data = getSNMP()

	mqttc = mosquitto.Mosquitto("Airport", True, data)
	mqttc.on_message = on_message
	mqttc.on_connect = on_connect
	mqttc.on_publish = on_publish
	mqttc.on_subscribe = on_subscribe
	mqttc.connect("192.168.1.10", 1883, 60)

	rc = mqttc.loop()
	while rc == 0 and len(waitingFor) > 0:
		logging.debug("main(): "+str(waitingFor))
		l = waitingFor[:]
		rc = mqttc.loop()
		if l == waitingFor and 'loop' in waitingFor:
			waitingFor.remove('loop')

	publish(mqttc, "airport", data)

	return 0

if __name__ == "__main__":
	sys.exit(main())
