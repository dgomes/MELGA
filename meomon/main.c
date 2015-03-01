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

#include "igmp.h"

#define HELLO_PORT 8082
#define HELLO_GROUP "239.255.255.250"
#define MSGBUFSIZE 1500

static int dbg_mode = 0;

struct ip_mreq group;

char * parse_ssdp(char *stream, char *buf) {
    xmlDoc         *document;
    xmlNode        *root, *node;

    stream[0] = '\0';

    if(dbg_mode) fprintf(stderr, "%s\n", buf);

    document = xmlRecoverMemory(buf, strlen(buf));

    root = xmlDocGetRootElement(document); // <node>
    root = root->children; // <activities>
    for (node = root->children; node; node = node->next) {
        if(xmlStrEqual(node->name, (xmlChar *) "tune")) {
            xmlChar *src = xmlGetProp(node, "src");
            strcpy(stream, (const char *) src);
            xmlFree(src);
            break;
        }
    }
    xmlFreeDoc(document);
    xmlCleanupParser();

    return stream;
}

int main(int argc, char *argv[])
{
    struct sockaddr_in addr;
    int fd, nbytes;
    struct ip_mreq mreq;
    char msgbuf[MSGBUFSIZE];

    struct mosquitto *mosq = NULL;
    mosquitto_lib_init();
    mosq = mosquitto_new(NULL, true, NULL);
    if(!mosq) {
        perror("mosquitto");
        exit(1);
    }

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
        printf("IP_ADD_MEMBERSHIP Ok\n");
    }

    /* now just enter a read-print loop */
    printf("GO!\n");

    if(IGMPv3_membership_report_message(HELLO_GROUP))
        exit(1);

    while (1) {
        socklen_t addrlen=sizeof(addr);
        if ((nbytes=recvfrom(fd,&msgbuf,MSGBUFSIZE,0, (struct sockaddr *) &addr,&addrlen)) < 0) {
            perror("recvfrom");
            exit(1);
        }
        char stream[255];
        parse_ssdp(stream, strchr(&msgbuf[49], '<'));
        fprintf(stderr, "%s\n", stream);
        bzero(msgbuf, MSGBUFSIZE);
    }
}

