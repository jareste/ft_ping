/*********************************/
/*            INCLUDE            */
/*********************************/

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
#include <pthread.h>

/*********************************/
/*            DEFINES            */
/*********************************/

#define PACKET_SIZE 64
#define TIMEOUT 1
#define SECONDS_TO_NANOSECONDS 1000 * 1000

typedef struct ping_args
{
    struct timeval* start;
    struct sockaddr_in* addr;
    int* transmitted;
    int* received;
    double* min_time;
    double* max_time;
    double* total_time;
    double* sum_sq_diff;
    double* avg_time;
    char* ip_str;
    int flags;
    int sockfd;
    int preload;
    int timeout_time;
    double interval;
} ping_args_t;

/*********************************/
/*            METHODS            */
/*********************************/

volatile int pinging = 1;

static void m_handle_signal(int sig)
{
    UNUSED_PARAM(sig);
    pinging = 0;
}

static unsigned short m_checksum(void *b, int len)
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

static void* m_send_ping(void *arg)
{
    struct icmp icmp_pkt;
    char sendbuf[PACKET_SIZE];
    int addrlen = sizeof(struct sockaddr_in);
    int seq = 0;
    /**/
    ping_args_t* args = (ping_args_t*)arg;
    struct timeval start = *args->start;
    struct sockaddr_in addr = *args->addr;
    int transmitted = *args->transmitted;
    int sockfd = args->sockfd;
    int preload = args->preload;
    int flags = args->flags;
    double interval = args->interval >= 0 ? args->interval : 1;
    if (interval < 0.0005)
    {
        interval = 0.0005;
    }

    while (pinging)
    {
        gettimeofday(&start, NULL);
        *args->start = start;

        memset(&icmp_pkt, 0, sizeof(icmp_pkt));
        icmp_pkt.icmp_type = ICMP_ECHO;
        icmp_pkt.icmp_code = 0;
        icmp_pkt.icmp_cksum = 0;
        icmp_pkt.icmp_seq = seq++;
        icmp_pkt.icmp_id = getpid();
        icmp_pkt.icmp_cksum = m_checksum(&icmp_pkt, sizeof(icmp_pkt));

        memcpy(sendbuf, &icmp_pkt, sizeof(icmp_pkt));

        if (sendto(sockfd, sendbuf, sizeof(icmp_pkt), 0, (struct sockaddr *)&addr, addrlen) <= 0)
        {
            /* TCP network may be colapsed, let it have some rest. */
            usleep(50);
            continue;
        }
        transmitted++;
        *args->transmitted = transmitted;
        if (flags & I_FLAG)
        {
            usleep(interval SECONDS_TO_NANOSECONDS);
        }
        else if (flags & F_FLAG)
        {
            /* even in flood mode, without usleep network colapses. */
            usleep(500);
        }
        else if ((preload <= 0))
        {
            usleep(interval SECONDS_TO_NANOSECONDS);
        }
        else if (preload > 0)
        {
            preload--;
        }
    }

    return NULL;

}

static void* m_receive_ping(void *arg)
{
    fd_set readfds;
    char recvbuf[PACKET_SIZE];
    socklen_t len = sizeof(struct sockaddr_in);
    struct timeval end, timeout;
    /**/
    ping_args_t* args = (ping_args_t*)arg;
    struct sockaddr_in addr = *args->addr;
    int sockfd = args->sockfd;
    char* ip_str = args->ip_str;
    int flags = args->flags;
    double min_time = *args->min_time;
    double max_time = *args->max_time;
    double total_time = *args->total_time;
    double avg_time = *args->avg_time;
    double sum_sq_diff = *args->sum_sq_diff;
    int received = *args->received;
    int timeout_time = args->timeout_time > 0 ? args->timeout_time : TIMEOUT;

    while (pinging)
    {
        timeout.tv_sec = timeout_time;
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

            gettimeofday(&end, NULL);

            struct timeval start = *args->start;

            double elapsed = (end.tv_sec - start.tv_sec) * 1000.0;
            elapsed += (end.tv_usec - start.tv_usec) / 1000.0;

            struct ip *ip_hdr = (struct ip *)recvbuf;
            struct icmp *recv_icmp = (struct icmp *)(recvbuf + (ip_hdr->ip_hl << 2));

            if (recv_icmp->icmp_type == ICMP_ECHOREPLY && recv_icmp->icmp_id == getpid())
            {
                received++;
                *args->received = received;
                
                if (!(flags & Q_FLAG))
                {
                    if (flags & D_FLAG)
                    {
                        printf("[%ld.%06ld]", end.tv_sec, end.tv_usec);
                    }

                    printf("%d bytes from %s: icmp_seq=%d ttl=%d time=%.3f ms\n",
                           PACKET_SIZE, ip_str, recv_icmp->icmp_seq, ip_hdr->ip_ttl, elapsed);
                }

                if (elapsed < min_time)
                {
                    min_time = elapsed;
                    *args->min_time = min_time;
                }
                if (elapsed > max_time)
                {
                    max_time = elapsed;
                    *args->max_time = max_time;
                }
                total_time += elapsed;
                *args->total_time = total_time;

                double diff = elapsed - avg_time;
                avg_time += diff / received;
                *args->avg_time = avg_time;
            
                sum_sq_diff += diff * (elapsed - avg_time);
                *args->sum_sq_diff = sum_sq_diff;
            }
        }
        else
        {
            /* select timeout, ignore!!! */
        }
    }

    return NULL;
}

/* public ping command entrypoint. */
void ping(const char *destination, int flags, int preload, int timeout_time, double interval)
{
    struct sockaddr_in addr;
    struct hostent *host;
    int sockfd;
    pthread_t recv_thread;
    pthread_t send_thread;
    struct timeval start, total_start, total_end;
    int transmitted = 0, received = 0;
    double min_time = INT_MAX, max_time = 0, total_time = 0;
    double sum_sq_diff = 0, avg_time = 0, mdev_time = 0;

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

    signal(SIGINT, m_handle_signal);

    gettimeofday(&total_start, NULL);

    if (flags & V_FLAG)
    {
        printf("ping: sock4.fd: %d (socktype: SOCK_RAW), sock6.fd: Not applies.\n\n", sockfd);
        printf("ai->ai_family: AF_INET, ai->ai_canonname: '%s'\n", destination);
    }

    printf("PING %s (%s) 56(84) bytes of data.\n", destination, ip_str);

    ping_args_t args = {
        .start = &start,
        .addr = &addr,
        .transmitted = &transmitted,
        .received = &received,
        .min_time = &min_time,
        .max_time = &max_time,
        .total_time = &total_time,
        .sum_sq_diff = &sum_sq_diff,
        .avg_time = &avg_time,
        .ip_str = ip_str,
        .flags = flags,
        .sockfd = sockfd,
        .preload = preload,
        .timeout_time = timeout_time,
        .interval = interval
    };

    pthread_create(&send_thread, NULL, m_send_ping, &args);
    pthread_create(&recv_thread, NULL, m_receive_ping, &args);

    pthread_join(send_thread, NULL);
    pthread_join(recv_thread, NULL);

    gettimeofday(&total_end, NULL);

    close(sockfd);

    printf("\n--- %s ping statistics ---\n", destination);
    printf("%d packets transmitted, %d received, %.0f%% packet loss, time %ldms\n",
           transmitted, received, (transmitted - received) / (double)transmitted * 100.0,
           (total_end.tv_sec - total_start.tv_sec) * 1000 + (total_end.tv_usec - total_start.tv_usec) / 1000);
    if (received > 0)
    {
        mdev_time = sqrt(sum_sq_diff / received);

        printf("rtt min/avg/max/mdev = %.3f/%.3f/%.3f/%.3f ms\n",
               min_time, total_time / received, max_time, mdev_time);
    }
}
