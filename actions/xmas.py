#!/usr/bin/python
#
# simple script that monitors light and power to decide to turn on the lights in the kitchen
#
import paho.mqtt.client as paho
import os
import time
import socket

#CONFIGURATION
broker = "localhost"
port = 1883

def action(luminosity, power):
#	print "luminosity = " + str(luminosity), ",", "power = " + str(power)
	if luminosity < 750:
		if power > 240:
#			print "Turn ON - XMAS"
			return "R00078043"
		else:
#			print "Turn OFF - XMAS"
			return "R00078042"
	else:
#		print "Turn OFF - XMAS"
		return "R00078042"

def on_connect(mqttc, userdata, rc):
	mqttc.subscribe([("imeter/power", 0), ("greenhouse/Luminosity", 0), ("weather/night", 0)])
 
def on_message(mqttc, userdata, message):
#	print message.topic + " -> " + str(message.payload)	
	userdata[message.topic] = message.payload

	cmd = ""
	
	if "weather/night" in userdata and "imeter/power" in userdata:
		if "True" in userdata["weather/night"]:
			cmd = action(0, int(userdata["imeter/power"]))
		else:
			cmd = action(1000, int(userdata["imeter/power"]))
	
	if len(cmd):
		hostname = socket.gethostname()
		mqttc.publish(hostname+"./ttyUSB0/cmd", cmd)
		mqttc.publish(hostname+"./ttyUSB1/cmd", cmd)
		mqttc.disconnect()
 
mypid = os.getpid()
client_uniq = "xmas_"+str(mypid)
userdata = dict()
mqttc = paho.Client(client_uniq, False, userdata) #nocleanstart

#define the callbacks
mqttc.on_message = on_message
mqttc.on_connect = on_connect
 
#connect to broker
mqttc.connect(broker, port, 60)
 
#remain connected and publish
while mqttc.loop() == 0:
	pass