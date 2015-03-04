#include <signal.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <time.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include <libxml/parser.h>

#include <mosquitto.h>

#include "utils.h"
#include "config.h"
#include "igmp.h"

#define NAME    "meostb"

#define HELLO_PORT 8082
#define HELLO_GROUP "239.255.255.250"
#define MSGBUFSIZE 1500

struct ip_mreq group;

char * parse_ssdp(char *stream, char *buf) {
    xmlDoc         *document;
    xmlNode        *root, *node;

    stream[0] = '\0';

    DBG("%s\n", buf);

    document = xmlRecoverMemory(buf, strlen(buf));

    root = xmlDocGetRootElement(document); // <node>
    root = root->children; // <activities>
    for (node = root->children; node; node = node->next) {
        if(xmlStrEqual(node->name, (xmlChar *) "tune")) {
            xmlChar *src = xmlGetProp(node, (const xmlChar *) "src");
            strcpy(stream, (const char *) src);
            xmlFree(src);
            break;
        }
    }
    xmlFreeNode(root);
    xmlFreeDoc(document);
    xmlCleanupParser();

    return stream;
}

int main(int argc, char *argv[])
{
    setbuf(stdout, NULL);
    INFO("%s v1.0\n", argv[0]);
    config_t cfg;
    struct mosquitto *mosq = NULL;

    config_init(&cfg);
    loadDefaults(&cfg);

    if(parseArgs(argc, argv, &cfg)) {
        exit(1);
    }

    struct sockaddr_in addr;
    int fd, nbytes;
    struct ip_mreq mreq;
    char msgbuf[MSGBUFSIZE];

    mosquitto_lib_init();
    mosq = mosquitto_new(NULL, true, NULL);
    if(!mosq) {
        perror("mosquitto");
        exit(1);
    }
    mqttserver_t mqtt;
    loadMQTT(&cfg, &mqtt);

    INFO("Connecting to %s:%d ... ", mqtt.servername, mqtt.port);
    if(mosquitto_connect(mosq, mqtt.servername, mqtt.port, mqtt.keepalive)){
        ERR("\nUnable to connect to %s:%d.\n", mqtt.servername, mqtt.port);
        exit(3);
    }
    INFO("done\n");

    if ((fd=socket(AF_INET,SOCK_DGRAM,0)) < 0) {
        perror("socket");
        exit(1);
    }

    /* allow multiple sockets to use the same PORT number */
    u_int optval=1;
    if (setsockopt(fd,SOL_SOCKET,SO_REUSEADDR,&optval,sizeof(optval)) < 0) {
        perror("Reusing ADDR failed");
        exit(1);
    }

    /* bind to receive address */
    memset(&addr,0,sizeof(addr));
    addr.sin_family=AF_INET;
    addr.sin_addr.s_addr=htonl(INADDR_ANY);
    addr.sin_port=htons(HELLO_PORT);
    if (bind(fd,(struct sockaddr *) &addr,sizeof(addr)) < 0) {
        perror("bind");
        exit(1);
    }

    /* use setsockopt() to request that the kernel join a multicast group */
    mreq.imr_multiaddr.s_addr=inet_addr(HELLO_GROUP);
    mreq.imr_interface.s_addr=htonl(INADDR_ANY);

    if (setsockopt(fd,IPPROTO_IP,IP_MULTICAST_IF,&mreq.imr_interface.s_addr,sizeof(struct in_addr)) < 0) {
        perror("setsockopt");
        exit(1);
    }

    if (setsockopt(fd,IPPROTO_IP,IP_ADD_MEMBERSHIP,&mreq,sizeof(mreq)) < 0) {
        perror("setsockopt");
        exit(1);
    } else {
        DBG("IP_ADD_MEMBERSHIP Ok\n");
    }

    /* now just enter a read-print loop */
    DBG("GO!\n");

    if(IGMPv3_membership_report_message(HELLO_GROUP))
        exit(1);


    int ts = (unsigned)time(NULL);
    while (1) {
        mosquitto_loop(mosq, -1, 1);
        socklen_t addrlen=sizeof(addr);
        if ((nbytes=recvfrom(fd,&msgbuf,MSGBUFSIZE,0, (struct sockaddr *) &addr,&addrlen)) < 0) {
            perror("recvfrom");
            exit(1);
        }
        char buf[255];
        parse_ssdp(buf, strchr(&msgbuf[49], '<'));

        char topic[255];
        snprintf(topic, 255, "%s/raw", NAME);
        DBG("publish to %s", topic);
        mosquitto_publish(mosq, NULL, topic, strlen(buf), strtok(buf,"\n"), 0, false);

        snprintf(topic, 255, "%s/ts", NAME);
        snprintf(buf, 255, "%d", (unsigned) time(NULL));
        mosquitto_publish(mosq, NULL, topic, strlen(buf), strtok(buf,"\n"), 0, true);

        bzero(msgbuf, MSGBUFSIZE);
        if((unsigned)time(NULL) - ts > IGMP_INTERVAL) {
            IGMPv3_membership_report_message(HELLO_GROUP);
            ts = (unsigned)time(NULL);
        }
    }
}

