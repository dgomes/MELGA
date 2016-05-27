#!/usr/bin/python
#
# simple script that monitors light and power to decide to turn on the lights in the xmas
#
# Andy Piper @andypiper http://andypiper.co.uk
# 
import paho.mqtt.client as paho
import os
import time
import socket

#CONFIGURATION
broker = "localhost"
port = 1883

def lights(turn=False):
	on = "R0002B358"
	off = "R0002B356"
	cmd = ""
	
	if turn:
		print "Turn on - Xmas"
		cmd = on
	else:
		print "Turn OFF - Xmas"
		cmd = off

	if len(cmd):
		hostname = socket.gethostname()
		mqttc.publish(hostname+"./ttyUSB0/cmd", cmd)
		mqttc.publish(hostname+"./ttyUSB1/cmd", cmd)
		mqttc.disconnect()

def on_connect(mqttc, userdata, rc):
	mqttc.subscribe([("imeter/power", 0), ("weather/night", 0), ("xmas/light", 0)])
 
def on_message(mqttc, userdata, message):
	print message.topic + " -> " + str(message.payload)	
	userdata[message.topic] = message.payload

	if "weather/night" in userdata and "imeter/power" in userdata and "xmas/light" in userdata:
		if "True" in userdata["weather/night"]:
			if int(userdata["imeter/power"]) > 240:
				lights(True)		
			elif userdata["xmas/light"] == True:
				mqttc.publish("xmas/light", "0", retain=True)
				mqttc.disconnect()
			else:
				lights(False)				
		else:
			lights(False)
	
 
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
