#ifndef FT_PING_H
#define FT_PING_H

#define UNUSED_PARAM(x) (void)(x)
#define V_FLAG 0x0000
#define F_FLAG 0x0001
#define Q_FLAG 0x0002
#define N_FLAG 0x0004
#define L_FLAG 0x0008
#define W_FLAG 0x0010


void ping(const char *destination, int flags, int preload, int timeout);

#define USAGE "\
Usage: ft_ping [options] <destination>\n\
\n\
Options:\n\
    -?              display this help message\n\
    -h              display this help message\n\
    -l  <preload>   send <preload> number of packages while waiting repliesn\
    -n              no reverse DNS name resolution\n\
    -q              quiet output\n\
    -v              verbose output\n\
    -f              flood ping\n\
    -W  <timeout>   time to wait for response\n\
"

#endif
