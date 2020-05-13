#include <malloc.h>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#define UDP_PORTNO           123
#define UNIX_TIMESTAMP_DELTA 2208988800ull

const char* usage = "./ntp-client [hostname]";

typedef struct {

    unsigned li : 2; //leap seconds indicator
    unsigned vn : 3; //protocol version number
    unsigned mode : 3; //client mode

    uint8_t  stratum; //stratum level of current clock
    uint8_t  poll;
    uint8_t  precision; //precision of current clock
    uint32_t rootDelay;
    uint32_t rootDispersion;
    uint32_t refId; //reference clock identifier

    uint32_t refTm_s;
    uint32_t refTm_f;

    uint32_t origTm_s;
    uint32_t origTm_f;

    uint32_t rxTm_s;
    uint32_t rxTm_f;

    uint32_t txTm_s; //time-stamp seconds
    uint32_t txTm_f; //time-stamp fraction of a second

} ntp_packet;

int main(int argc, char* argv[])
{
    ntp_packet*        packet = NULL;
    char*              hostname;
    int                sock, msg;
    struct sockaddr_in addr;
    struct hostent*    server_info;

    if (argc != 2) {
        printf("%s\n", usage);
        return -1;
    }

    hostname = argv[1];

    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;

    packet = (ntp_packet*)calloc(1, sizeof(ntp_packet));
    if (packet == NULL) {
        printf("Not enough memory\n");
        return 1;
    }
    *((char*)packet) = 0x1b;

    server_info = gethostbyname(hostname);
    if (server_info == NULL) {
        printf("Invalid hostname\n");
        return 1;
    }

    sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (sock < 0) {
        printf("Cannot open socket\n");
        return 2;
    }
    strncpy((char*)server_info->h_addr, (char*)&addr.sin_addr.s_addr, server_info->h_length);
    addr.sin_port = htons(UDP_PORTNO);

    if (connect(sock, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        printf("Cannot connect\n");
        return 2;
    }

    msg = write(sock, (char*)packet, sizeof(ntp_packet));
    if (msg < 0) {
        printf("Cannot write to socket\n");
        return 2;
    }

    msg = read(sock, (char*)packet, sizeof(ntp_packet));
    if (msg < 0) {
        printf("Cannot read from socket\n");
        return 2;
    }

    packet->txTm_f = ntohl(packet->txTm_f);
    packet->txTm_s = ntohl(packet->txTm_s);

    time_t final = (time_t)(packet->txTm_s - UNIX_TIMESTAMP_DELTA);

    printf("Current time: %s", ctime((const time_t*)&final));

    free(packet);
    return 0;
}
