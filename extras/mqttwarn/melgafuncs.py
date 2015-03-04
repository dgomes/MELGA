def static_var(varname, value):
	def decorate(func):
		setattr(func, varname, value)
		return func
	return decorate

# Filter
def code200(topic, message):
	import json
	try:
		data = json.loads(message.rstrip('\x00'))
		if "code" in data and data["code"] == 200:
			return False
	except:
		pass
	return True

def doorOpen(topic, message):
	if message == "0x7F7B04": # packet = door sensor
    		return hysteresis(topic, message)
	return True

@static_var("h_db", dict())
def hysteresis(topic, message):
	import time
	ts = time.time()
	#print "TS = " + str(ts)			
	if message in hysteresis.h_db:
		if ts - hysteresis.h_db[message] < 10:
			print "ignore DB " + message + " = " + str(ts) + " we have " + str(hysteresis.h_db[message])
			return True
	hysteresis.h_db[message] = ts
	print "DB " + message + " = " + str(hysteresis.h_db[message])
	return False 

def greenHouseDead(topic, message):
	import datetime
	now = datetime.datetime.now()
	midnight = now.replace(hour= 0, minute=0, second=0, microsecond=0)
	midnightplus = now.replace(hour= 0, minute=10, second=0, microsecond=0)
	noon = now.replace(hour= 12, minute=0, second=0, microsecond=0)
	noonplus = now.replace(hour= 12, minute=10, second=0, microsecond=0)
	if int(message) > 500 and now > midnight and now < midnightplus:
		return False
	if int(message) < 500 and now > noon and now < noonplus:
		return False
	return True

# 
def toMBitsS(data, srv=None):
	if type(data) == dict and "payload" in data:
		return str(int(data["payload"]) * 8 / 1024)
	return None

