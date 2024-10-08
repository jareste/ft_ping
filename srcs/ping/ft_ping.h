#ifndef FT_PING_H
#define FT_PING_H

#define UNUSED_PARAM(x) (void)(x)
#define F_FLAG 0x0001
#define Q_FLAG 0x0002
#define N_FLAG 0x0004
#define L_FLAG 0x0008
#define W_FLAG 0x0010
#define V_FLAG 0x0020
#define D_FLAG 0x0040
#define I_FLAG 0x0080
#define T_FLAG 0x0100

void ping(const char *destination, int flags, int preload, int timeout_time, double interval, int ttl);

#define USAGE "\
Usage: ft_ping [options] <destination>\n\
\n\
Options:\n\
    -?              display this help message\n\
    -h              display this help message\n\
    -l  <preload>   send <preload> number of packages while waiting replies\n\
    -n              no reverse DNS name resolution\n\
    -q              quiet output\n\
    -v              verbose output\n\
    -f              flood ping (overrides 'i'\n\
    -D              print timestamps\n\
    -t  <ttl>       define time to live\n\
    -i  <interval>  seconds between sending each packet\n\
    -W  <timeout>   time to wait for response\n\
"

#endif
