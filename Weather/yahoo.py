#!/usr/bin/python 
import urllib2, urllib, json
import datetime
# 
import paho.mqtt.client as paho
import os
import time

#CONFIGURATION
from pylibconfig2 import Config

cfg = None
with open ("/etc/melga.cfg", "r") as conffile:
    cfg = Config(conffile.read())

def getAstronomy():
	baseurl = "https://query.yahooapis.com/v1/public/yql?"
	yql_query = "select astronomy from weather.forecast where woeid = "+str(cfg.yahoo.woeid)
	yql_url = baseurl + urllib.urlencode({'q':yql_query}) + "&format=json"
	result = urllib2.urlopen(yql_url).read()
	data = json.loads(result)
	astronomy = data['query']['results']['channel']['astronomy'] 
	sunset = datetime.datetime.strptime(datetime.date.today().strftime("%d/%m/%Y") + " " + astronomy['sunset'], "%d/%m/%Y %I:%M %p")
	sunrise = datetime.datetime.strptime(datetime.date.today().strftime("%d/%m/%Y") + " " + astronomy['sunrise'], "%d/%m/%Y %I:%M %p")

	return sunrise, sunset

def on_connect(mqttc, userdata, rc):
	sunrise, sunset = getAstronomy()
	now = datetime.datetime.now()

	mqttc.publish("weather/night", now < sunrise or now > sunset, retain=True)
	mqttc.publish("weather/day", now > sunrise and now < sunset, retain=True)
	
	mqttc.disconnect()

mypid = os.getpid()
client_uniq = "weather_"+str(mypid)
userdata = dict()
mqttc = paho.Client(client_uniq, False, userdata) #nocleanstart

#connect to broker
mqttc.connect(cfg.mqtt.server, cfg.mqtt.port, 60)
mqttc.on_connect = on_connect

#remain connected and publish
while mqttc.loop() == 0:
        pass



