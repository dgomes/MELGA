 /*
 * Based on the work of Thomas Bernard in upnpc
 * Copyright (c) 2014 Diogo Gomes
 * Copyright (c) 2005-2013 Thomas Bernard
 * This software is subject to the conditions detailed in the
 * LICENCE file provided in this distribution. */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
/* for IPPROTO_TCP / IPPROTO_UDP */
#include <netinet/in.h>
#include <miniwget.h>
#include <miniupnpc.h>
#include <upnpcommands.h>
#include <upnperrors.h>
#include <utils.h>
#include <mosquitto.h>

struct state {
	unsigned bytessent;
	unsigned bytesreceived;
	unsigned packetsreceived;
	unsigned packetssent;

	unsigned bytessent_per_second;
	unsigned bytesreceived_per_second;
	time_t now;	
};

void GetConnectionStatus(struct state *s, struct UPNPUrls * urls, struct IGDdatas * data); 
