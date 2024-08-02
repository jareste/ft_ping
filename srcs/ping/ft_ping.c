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
#include <signal.h>
#include <limits.h>
#include <math.h>

#define PACKET_SIZE 64
#define TIMEOUT 1

unsigned short checksum(void *b, int len)
{
    unsigned short *buf = b;
    unsigned int sum = 0;
    unsigned short result;

    for (sum = 0; len > 1; len -= 2)
    {
        sum += *buf++;
    }

    if (len == 1)
    {
        sum += *(unsigned char *)buf;
    }

    sum = (sum >> 16) + (sum & 0xFFFF);
    sum += (sum >> 16);
    result = ~sum;
    return result;
}

volatile int pinging = 1;

void handle_signal(int sig)
{
    UNUSED_PARAM(sig);
    pinging = 0;
}

void ping(const char *destination, int flags, int preload, int timeout_time)
{
    struct sockaddr_in addr;
    struct hostent *host;
    int sockfd;
    struct icmp icmp_pkt;
    char sendbuf[PACKET_SIZE];
    char recvbuf[PACKET_SIZE];
    int addrlen = sizeof(addr);
    socklen_t len = sizeof(addr);
    struct timeval start, end, total_start, total_end;
    int seq = 0;
    int transmitted = 0, received = 0;
    double min_time = INT_MAX, max_time = 0, total_time = 0;
    double sum_sq_diff = 0, avg_time = 0, mdev_time = 0;
    struct timeval timeout;
    fd_set readfds;

    UNUSED_PARAM(preload);
    UNUSED_PARAM(timeout_time);

    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;

    if ((host = gethostbyname(destination)) == NULL)
    {
        perror("gethostbyname");
        exit(EXIT_FAILURE);
    }
    memcpy(&addr.sin_addr, host->h_addr, host->h_length);

    char ip_str[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &(addr.sin_addr), ip_str, INET_ADDRSTRLEN);

    if ((sockfd = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP)) < 0)
    {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    signal(SIGINT, handle_signal);

    gettimeofday(&total_start, NULL);

    while (pinging)
    {
        gettimeofday(&start, NULL);

        memset(&icmp_pkt, 0, sizeof(icmp_pkt));
        icmp_pkt.icmp_type = ICMP_ECHO;
        icmp_pkt.icmp_code = 0;
        icmp_pkt.icmp_cksum = 0;
        icmp_pkt.icmp_seq = seq++;
        icmp_pkt.icmp_id = getpid();
        icmp_pkt.icmp_cksum = checksum(&icmp_pkt, sizeof(icmp_pkt));

        memcpy(sendbuf, &icmp_pkt, sizeof(icmp_pkt));

        if (sendto(sockfd, sendbuf, sizeof(icmp_pkt), 0, (struct sockaddr *)&addr, addrlen) <= 0)
        {
            perror("sendto");
            exit(EXIT_FAILURE);
        }
        transmitted++;

        if (flags & V_FLAG)
        {
            printf("Sent ICMP packet to %s\n", destination);
        }

        timeout.tv_sec = TIMEOUT;
        timeout.tv_usec = 0;
        FD_ZERO(&readfds);
        FD_SET(sockfd, &readfds);

        int ret = select(sockfd + 1, &readfds, NULL, NULL, &timeout);
        if (ret > 0 && FD_ISSET(sockfd, &readfds))
        {
            if (recvfrom(sockfd, recvbuf, sizeof(recvbuf), 0, (struct sockaddr *)&addr, &len) <= 0)
            {
                perror("recvfrom");
                continue;
            }
            received++;

            gettimeofday(&end, NULL);

            double elapsed = (end.tv_sec - start.tv_sec) * 1000.0;
            elapsed += (end.tv_usec - start.tv_usec) / 1000.0;

            struct ip *ip_hdr = (struct ip *)recvbuf;
            struct icmp *recv_icmp = (struct icmp *)(recvbuf + (ip_hdr->ip_hl << 2));

            if (recv_icmp->icmp_type == ICMP_ECHOREPLY && recv_icmp->icmp_id == getpid())
            {
                if (!(flags & Q_FLAG))
                {
                    printf("%d bytes from %s: icmp_seq=%d ttl=%d time=%.1f ms\n",
                           PACKET_SIZE, ip_str, recv_icmp->icmp_seq, ip_hdr->ip_ttl, elapsed);
                }

                if (elapsed < min_time)
                    min_time = elapsed;
                if (elapsed > max_time)
                    max_time = elapsed;
                total_time += elapsed;

                double diff = elapsed - avg_time;
                avg_time += diff / received;
                sum_sq_diff += diff * (elapsed - avg_time);
            }
        }
        else
        {
            /*timeout*/
        }

        gettimeofday(&end, NULL);
        double time_spent = (end.tv_sec - start.tv_sec) * 1000.0 + (end.tv_usec - start.tv_usec) / 1000.0;
        if (time_spent < 1000.0)
        {
            usleep((1000.0 - time_spent) * 1000);
        }
    }

    gettimeofday(&total_end, NULL);

    close(sockfd);

    mdev_time = sqrt(sum_sq_diff / received);

    printf("\n--- %s ping statistics ---\n", destination);
    printf("%d packets transmitted, %d received, %.0f%% packet loss, time %ldms\n",
           transmitted, received, (transmitted - received) / (double)transmitted * 100.0,
           (total_end.tv_sec - total_start.tv_sec) * 1000 + (total_end.tv_usec - total_start.tv_usec) / 1000);
    if (received > 0)
    {
        printf("rtt min/avg/max/mdev = %.3f/%.3f/%.3f/%.3f ms\n",
               min_time, total_time / received, max_time, mdev_time);
    }
}
