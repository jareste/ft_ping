#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ft_ping.h>

void print_usage()
{
    /*TODO improve usage, kind of cutre but copilot tab*/
    printf("Usage: ft_ping [-v] [-?] <destination>\n");
}

void parse_argv(int argc, char *argv[], int *verbose, char **destination)
{
    int opt;

    while ((opt = getopt(argc, argv, "v?")) != -1)
    {
        switch (opt)
        {
            case 'v':
                *verbose = 1;
                break;
            case '?':
                print_usage();
                exit(0);
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
    int verbose = 0;
    char *destination = NULL;

    parse_argv(argc, argv, &verbose, &destination);

    if (verbose)
    {
        printf("PING %s\n", destination);
    }

    ping(destination, verbose);

    return 0;
}
