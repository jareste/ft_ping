#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ft_ping.h>

void print_usage()
{
    printf(USAGE);
}

void parse_argv(int argc, char *argv[], int *flags, char **destination, int *preload, time_t *timeout)
{
    int opt;

    while ((opt = getopt(argc, argv, "v?hlnqfW")) != -1)
    {
        switch (opt)
        {
            case 'v':
                *flags |= V_FLAG;
                break;
            case '?':
                print_usage();
                exit(0);
            case 'h':
                print_usage();
                exit(0);
            case 'l':
                *flags |= L_FLAG;
                *preload = atoi(optarg);
                break;
            case 'n':
                *flags |= N_FLAG;
                break;
            case 'q':
                *flags |= Q_FLAG;
                break;
            case 'f':
                *flags |= F_FLAG;
                break;
            case 'W':
                *flags |= W_FLAG;
                *timeout = atoi(optarg);
                break;
            default:
                print_usage();
                exit(1);
        }
    }

    if (optind >= argc)
    {
        fprintf(stderr, "Expected argument after options\n");
        print_usage();
        exit(1);
    }

    *destination = argv[optind];
}

int main(int argc, char *argv[])
{
    int flags = 0;
    char *destination = NULL;
    int preload = 0;
    time_t timeout = 0;

    parse_argv(argc, argv, &flags, &destination, &preload, &timeout);

    if (flags & V_FLAG)
    {
        printf("PING %s\n", destination);
    }

    ping(destination, flags, preload, timeout);

    return 0;
}
