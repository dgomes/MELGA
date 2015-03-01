#ifndef _IGMP_H_
#define _IGMP_H_

#include <sys/socket.h>
#include <netinet/in.h>
#include <stdint.h>
#include <unistd.h>
#include <stdio.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>

#include "utils.h"

#define IGMP_INTERVAL  500

struct __attribute__((packed)) group_record {
    uint8_t type;
    uint8_t len;
    uint16_t number_sources;
    in_addr_t multicast_address;
    in_addr_t source_address[0];
};

struct __attribute__((packed)) igmp3 {
    uint8_t type;
    uint8_t max_resp;
    uint16_t crc;
    uint16_t reserved;
    uint16_t numbergroups;
    struct group_record gr[0];
};
int IGMPv3_membership_report_message(char *multicast_group);

#endif
