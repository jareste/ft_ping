#include "ft_ping.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/ip_icmp.h>
#include <errno.h>
#include <sys/time.h>

#define PACKET_SIZE 64
#define INT_MAX 2147483647
#define PING_INTERVAL 1

unsigned short checksum(void *b, int len)
{    
    unsigned short *buf = b; 
    unsigned int sum=0;
    unsigned short result;

    for (sum = 0; len > 1; len -= 2)
    {
        sum += *buf++;
    }

    if (len == 1)
    {
        sum += *(unsigned char*)buf;
    }

    sum = (sum >> 16) + (sum & 0xFFFF);
    sum += (sum >> 16);
    result = ~sum;
    return result;
}

void ping(const char *destination, int verbose)
{
    struct sockaddr_in addr;
    struct hostent *host;
    int sockfd;
    struct icmp icmp_pkt;
    char sendbuf[PACKET_SIZE];
    char recvbuf[PACKET_SIZE];
    int addrlen = sizeof(addr);
    socklen_t len = sizeof(addr);
    struct timeval start, end;

    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;

    if ((host = gethostbyname(destination)) == NULL)
    {
        perror("gethostbyname");
        exit(EXIT_FAILURE);
    }
    memcpy(&addr.sin_addr, host->h_addr, host->h_length);

    if ((sockfd = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP)) < 0)
    {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    for (int seq = 0; seq < INT_MAX; ++seq)
    {
        memset(&icmp_pkt, 0, sizeof(icmp_pkt));
        icmp_pkt.icmp_type = ICMP_ECHO;
        icmp_pkt.icmp_code = 0;
        icmp_pkt.icmp_cksum = 0;
        icmp_pkt.icmp_seq = seq;
        icmp_pkt.icmp_id = getpid();
        icmp_pkt.icmp_cksum = checksum(&icmp_pkt, sizeof(icmp_pkt));

        memcpy(sendbuf, &icmp_pkt, sizeof(icmp_pkt));

        gettimeofday(&start, NULL);

        if (sendto(sockfd, sendbuf, sizeof(icmp_pkt), 0, (struct sockaddr *)&addr, addrlen) <= 0)
        {
            perror("sendto");
            exit(EXIT_FAILURE);
        }

        if (verbose)
        {
            printf("Sent ICMP packet to %s\n", destination);
        }

        if (recvfrom(sockfd, recvbuf, sizeof(recvbuf), 0, (struct sockaddr *)&addr, &len) <= 0)
        {
            perror("recvfrom");
            exit(EXIT_FAILURE);
        }

        gettimeofday(&end, NULL);

        double elapsed = (end.tv_sec - start.tv_sec) * 1000.0;
        elapsed += (end.tv_usec - start.tv_usec) / 1000.0;

        struct icmp *recv_icmp = (struct icmp *)(recvbuf + sizeof(struct ip));
        if (recv_icmp->icmp_type == ICMP_ECHOREPLY && recv_icmp->icmp_id == getpid())
        {
            printf("64 bytes from %s: icmp_seq=%d ttl=%d time=%.1f ms\n",
                   destination, seq, 64, elapsed);
        }

        sleep(PING_INTERVAL);
    }

    close(sockfd);
}
