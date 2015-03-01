#include "igmp.h"


/* https://tools.ietf.org/html/rfc1071 */
uint16_t checksum(void* vdata,size_t length) {
    register uint32_t sum = 0;
    uint16_t *d = vdata;

    for(int i=0; i< length/2; i++) {
        sum+=d[i];
    }

    while (sum>>16)
        sum = (sum & 0xffff) + (sum >> 16);

    return ~sum;
}

char *multicast_address = NULL;

void refresh_igmp(int s) {
    fprintf(stderr, "what's up?\n");
    IGMPv3_membership_report_message(multicast_address);
}

int IGMPv3_membership_report_message(char *mc_addr) {
    if(multicast_address == NULL) {
        multicast_address = (char *) malloc(strlen(mc_addr)+1);
        strcpy(multicast_address, mc_addr);
    }
    int ret = 0;
    int s = socket(AF_INET, SOCK_RAW, IPPROTO_IGMP);
    void *p = malloc(sizeof(struct igmp3) + sizeof(struct group_record) );
    memset(p, 0, sizeof(struct igmp3) + sizeof(struct group_record) );
    struct igmp3 *ig = (struct igmp3 *) p;
    struct sockaddr_in dst;
    dst.sin_family = AF_INET;
    dst.sin_addr.s_addr = inet_addr("224.0.0.22");
    ig->type = 0x22;
    ig->numbergroups = htons(0x1);
    ig->gr[0].type = 0x2;
    ig->gr[0].len = 0;
    ig->gr[0].number_sources = 0;
    ig->gr[0].multicast_address = inet_addr(mc_addr);

    ig->crc = checksum(p, sizeof(struct igmp3) + sizeof(struct group_record) );

    if (s < 0) {
        perror("socket");
        fprintf(stderr, "Do you have privileges to use SOCK_RAW?\n");
        ret = 1;
        goto exit;
    }
    if(sendto(s, ig, sizeof(struct igmp3) + sizeof(struct group_record), 0, (struct sockaddr *)&dst, sizeof(struct sockaddr_in)) < 0) {
        perror("sendto");
        ret = 2;
        goto exit;
    }
    signal(SIGALRM, refresh_igmp);
    alarm(IGMP_INTERVAL);

exit:
    free(p);
    return ret;
}

