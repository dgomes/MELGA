#include "igd.h"

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
	DBG("Bytes:   Sent: %8u\tRecv: %8u\n", s->bytessent, s->bytesreceived);
	DBG("Packets: Sent: %8u\tRecv: %8u\n", s->packetssent, s->packetsreceived);
}

/* sample upnp client program */
int main(int argc, char ** argv)
{
	time_t pool_interval = 180;
	struct UPNPDev * devlist = 0;
	char lanaddr[64];	/* my ip address on the LAN */
	int i;
	const char * rootdescurl = 0;
	const char * multicastif = 0;
	const char * minissdpdpath = 0;
	int retcode = 0;
	int error = 0;

	char id[30] = "igd"; //TODO randomize this else broker will keep us disconnecting...
        DBG("UPnP IGD Publisher v1\n");
        //TODO read this from the configuration file
        char *host = "192.168.1.10";
        int port = 1883;
        int keepalive = 60;
        bool clean_session = true;
        struct mosquitto *mosq = NULL;

	mosquitto_lib_init();

	mosq = mosquitto_new(id, clean_session, NULL);

	if(mosquitto_connect(mosq, host, port, keepalive)){
                ERR("Unable to connect.");
                return(EXIT_FAILURE);
        }

	if( rootdescurl || (devlist = upnpDiscover(2000, multicastif, minissdpdpath, 0/*sameport*/))) {
		struct UPNPUrls urls;
		struct IGDdatas data;
		if(!devlist) {
			ERR("upnpDiscover() error code=%d\n", error);
			exit(EXIT_FAILURE);
		}
		i = 1;
		if( (rootdescurl && UPNP_GetIGDFromUrl(rootdescurl, &urls, &data, lanaddr, sizeof(lanaddr))) || (i = UPNP_GetValidIGD(devlist, &urls, &data, lanaddr, sizeof(lanaddr)))) {
			switch(i) {
			case 1:
				INFO("Found valid IGD : %s\n", urls.controlURL);
				break;
			case 2:
				ERR("Found a (not connected?) IGD : %s\n", urls.controlURL);
				break;
			case 3:
				ERR("UPnP device found. Is it an IGD ? : %s\n", urls.controlURL);
				break;
			default:
				ERR("Found device (igd ?) : %s\n", urls.controlURL);
			}

			struct state cur;
			int rc;
			cur.now = 0;
			while((rc = mosquitto_loop(mosq, -1, 1)) == MOSQ_ERR_SUCCESS) {
				if(time(NULL) >= cur.now + pool_interval) {
					GetConnectionStatus(&cur, &urls, &data);

					char topic[255];
					char payload[10];
					sprintf(topic, "%s/%s", "igd", "outBytesSecond"); 	
					sprintf(payload,"%u", cur.bytessent_per_second);
			                mosquitto_publish(mosq, NULL, topic, strlen(payload), payload, 0, true);
					sprintf(topic, "%s/%s", "igd", "inBytesSecond"); 	
					sprintf(payload,"%u", cur.bytesreceived_per_second);
			                mosquitto_publish(mosq, NULL, topic, strlen(payload), payload, 0, true);
					sprintf(topic, "%s/%s", "igd", "inBytes"); 	
					sprintf(payload,"%u", cur.bytesreceived);
			                mosquitto_publish(mosq, NULL, topic, strlen(payload), payload, 0, true);
					sprintf(topic, "%s/%s", "igd", "outBytes"); 	
					sprintf(payload,"%u", cur.bytessent);
			                mosquitto_publish(mosq, NULL, topic, strlen(payload), payload, 0, true);
					char *raw;
					asprintf(&raw, "{\"id\": \"igd\", \"outBytesSecond\": %u, \"inBytesSecond\": %u, \"inBytes\": %u, \"outBytes\": %u}", cur.bytessent_per_second, cur.bytesreceived_per_second, cur.bytesreceived, cur.bytessent);
					sprintf(topic, "%s/%s", "igd", "raw"); 	
			                mosquitto_publish(mosq, NULL, topic, strlen(raw), raw, 0, false);
					free(raw);
				}
			}

			FreeUPNPUrls(&urls);
		} else {
			ERR("No valid UPNP Internet Gateway Device found.\n");
			retcode = 1;
		}
		freeUPNPDevlist(devlist); devlist = 0;
	} else {
		ERR("No IGD UPnP Device found on the network !\n");
		retcode = 1;
	}
        /* Cleanup */
        mosquitto_destroy(mosq);
        mosquitto_lib_cleanup();
	return retcode;
}

