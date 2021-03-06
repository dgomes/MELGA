#include "igd.h"
#include "config.h"

void GetConnectionStatus(struct state *s, struct UPNPUrls * urls, struct IGDdatas * data) {

	unsigned last_bytessent = s->bytessent;
	unsigned last_bytesreceived = s->bytesreceived;
	unsigned last_time = s->now;

	//	DisplayInfos(urls, data);
	s->bytessent = UPNP_GetTotalBytesSent(urls->controlURL_CIF, data->CIF.servicetype);
	s->bytesreceived = UPNP_GetTotalBytesReceived(urls->controlURL_CIF, data->CIF.servicetype);
	s->packetssent = UPNP_GetTotalPacketsSent(urls->controlURL_CIF, data->CIF.servicetype);
	s->packetsreceived = UPNP_GetTotalPacketsReceived(urls->controlURL_CIF, data->CIF.servicetype);
	s->now = time(NULL);

	if(last_time != 0) {
		s->bytessent_per_second = (s->bytessent - last_bytessent)/(s->now - last_time);
		s->bytesreceived_per_second = (s->bytesreceived - last_bytesreceived)/(s->now - last_time);
	} else {
		s->bytessent_per_second = 0;
		s->bytesreceived_per_second = 0;
	}

	DBG("Rates:   Upload: %8u\tDownload: %8u\n", s->bytessent_per_second, s->bytesreceived_per_second);
	DBG("Rates:   Upload: %8.2f kbits\tDownload: %8.2f kbits\n", ((double) s->bytessent_per_second)/128, ((double)s->bytesreceived_per_second)/128);
	DBG("Bytes:   Sent: %8u\tRecv: %8u\n", s->bytessent, s->bytesreceived);
	DBG("Packets: Sent: %8u\tRecv: %8u\n", s->packetssent, s->packetsreceived);
}

unsigned FindIGD( struct UPNPUrls *urls, struct IGDdatas *data) {
	int retcode = 0;
	char lanaddr[64];	/* my ip address on the LAN */
	int i;
	int error = 0;
	struct UPNPDev * devlist = 0;
	const char * rootdescurl = 0;
	const char * multicastif = 0;
	const char * minissdpdpath = 0;
#ifdef MINIUPNPC_API_VERSION
	if( rootdescurl || (devlist = upnpDiscover(2000, multicastif, minissdpdpath, 0/*sameport*/, 0, NULL))) {
#else
	if( rootdescurl || (devlist = upnpDiscover(2000, multicastif, minissdpdpath, 0/*sameport*/))) {
#endif
		if(!devlist) {
			ERR("upnpDiscover() error code=%d\n", error);
			exit(EXIT_FAILURE);
		}
		i = 1;
		if( (rootdescurl && UPNP_GetIGDFromUrl(rootdescurl, urls, data, lanaddr, sizeof(lanaddr))) || (i = UPNP_GetValidIGD(devlist, urls, data, lanaddr, sizeof(lanaddr)))) {
			switch(i) {
				case 1:
					INFO("Found valid IGD : %s\n", urls->controlURL);
					break;
				case 2:
					ERR("Found a (not connected?) IGD : %s\n", urls->controlURL);
					break;
				case 3:
					ERR("UPnP device found. Is it an IGD ? : %s\n", urls->controlURL);
					break;
				default:
					ERR("Found device (igd ?) : %s\n", urls->controlURL);
			}
		} else {
			ERR("No valid UPNP Internet Gateway Device found.\n");
			retcode = 2;
		}
	} else {
		ERR("No IGD UPnP Device found on the network !\n");
		retcode = 1;
	}

	if(retcode > 0)
		freeUPNPDevlist(devlist);
	if(retcode > 1)
		FreeUPNPUrls(urls);

	return retcode;
}

/* sample upnp client program */
int main(int argc, char ** argv)
{
	time_t pool_interval = 180; //TODO command line parameter ...

	config_t cfg;
	config_init(&cfg);
	loadDefaults(&cfg);
	if(parseArgs(argc, argv, &cfg)) {
		exit(1);
	}

	mqttserver_t mqtt;
	loadMQTT(&cfg, &mqtt);


	INFO("UPnP IGD Publisher v2\n");
	//TODO read this from the configuration file
	struct mosquitto *mosq = NULL;

	mosquitto_lib_init();

	mosq = mosquitto_new(NULL, true, NULL);

	unsigned backoff_time = 2;
	while(mosquitto_connect(mosq, mqtt.servername, mqtt.port, mqtt.keepalive)){
			ERR("Unable to connect to <%s:%d>", mqtt.servername, mqtt.port);
			backoff_time = backoff(backoff_time);
			if(!backoff_time)
				return(EXIT_FAILURE);
	}

	INFO("connected to MQTT broker - %s\n", mqtt.servername);

	struct UPNPUrls urls;
   	struct IGDdatas data;
		
	while(FindIGD(&urls, &data) > 2) {
			backoff_time = backoff(backoff_time);
			if(!backoff_time)
				return(EXIT_FAILURE);
	}

	unsigned retcode = 0;	
	struct state cur;
	cur.now = 0;
	while((retcode = mosquitto_loop(mosq, -1, 1)) == MOSQ_ERR_SUCCESS) {
		if(time(NULL) >= cur.now + pool_interval) {
			GetConnectionStatus(&cur, &urls, &data);

			char topic[255];
			char payload[63];
			sprintf(topic, "%s/%s", "igd", "outBytesSecond");
			sprintf(payload,"%u", cur.bytessent_per_second);
			mosquitto_publish(mosq, NULL, topic, strlen(payload), payload, 0, true);
			sprintf(topic, "%s/%s", "igd", "inBytesSecond");
			sprintf(payload,"%u", cur.bytesreceived_per_second);
			mosquitto_publish(mosq, NULL, topic, strlen(payload), payload, 0, true);
			sprintf(topic, "%s/%s", "igd", "uploadRate");
			sprintf(payload,"%.0f", ((double) cur.bytessent_per_second)/128);
			mosquitto_publish(mosq, NULL, topic, strlen(payload), payload, 0, false);
			sprintf(topic, "%s/%s", "igd", "downloadRate");
			sprintf(payload,"%.0f", ((double) cur.bytesreceived_per_second)/128);
			mosquitto_publish(mosq, NULL, topic, strlen(payload), payload, 0, false);
			sprintf(topic, "%s/%s", "igd", "inBytes");
			sprintf(payload,"%u", cur.bytesreceived);
			mosquitto_publish(mosq, NULL, topic, strlen(payload), payload, 0, true);
			sprintf(topic, "%s/%s", "igd", "outBytes");
			sprintf(payload,"%u", cur.bytessent);
			mosquitto_publish(mosq, NULL, topic, strlen(payload), payload, 0, true);
			char *raw;
			asprintf(&raw, "{\"id\": \"igd\", \"outBytesSecond\": %u, \"inBytesSecond\": %u, \"inBytes\": %u, \"outBytes\": %u}", cur.bytessent_per_second, cur.bytesreceived_per_second, cur.bytesreceived, cur.bytessent);
			sprintf(topic, "%s/%s", "igd", "raw");
			DBG("Publish %s -> %s\n", raw, topic);
			mosquitto_publish(mosq, NULL, topic, strlen(raw), raw, 0, false);
			free(raw);
		}
	}
	/* Cleanup */
	mosquitto_destroy(mosq);
	mosquitto_lib_cleanup();
	INFO("%s EXITED", argv[0]);
	return retcode;
}

