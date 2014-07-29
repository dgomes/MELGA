def static_var(varname, value):
	def decorate(func):
		setattr(func, varname, value)
		return func
	return decorate

@static_var("prev", [])
def doorOpen(topic, message):
	#print message
	#print doorOpen.prev
	if message in doorOpen.prev:
		return True
	if len(doorOpen.prev) > 5:
		del doorOpen.prev[0]
	doorOpen.prev.append(message)
	if message == "0x7F7B04": # packet = door sensor
   		return False
    	return True

def greenHouseDead(topic, message):
	import datetime
	now = datetime.datetime.now()
	midnight = now.replace(hour= 0, minute=0, second=0, microsecond=0)
	midnightplus = now.replace(hour= 0, minute=5, second=0, microsecond=0)
	noon = now.replace(hour= 12, minute=0, second=0, microsecond=0)
	noonplus = now.replace(hour= 12, minute=5, second=0, microsecond=0)
	if int(message) > 500 and now > midnight and now < midnightplus:
		return False
	if int(message) < 500 and now > noon and now < noonplus:
		return False
	return True

def toMBitsS(data, srv=None):
	if type(data) == dict and "payload" in data:
		return str(int(data["payload"]) * 8 / 1024)
	return None
