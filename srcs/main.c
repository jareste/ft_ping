#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ft_ping.h>
#include <ctype.h>

void print_usage()
{
    printf(USAGE);
}

void parse_argv(int argc, char *argv[], int *flags, char **destination, int *preload, time_t *timeout, double* interval)
{
    int opt;

    while ((opt = getopt(argc, argv, "v?hl:nqfiDW:")) != -1)
    {
        switch (opt)
        {
            case 'v':
                *flags |= V_FLAG;
                break;
            case '?':
            case 'h':
                print_usage();
                exit(0);
            case 'l':
                *flags |= L_FLAG;
                if (optarg && isdigit(optarg[0]))
                {
                    *preload = atoi(optarg);
                }
                else
                {
                    fprintf(stderr, "Option -l contains garbage as argument: %s.\n", optarg);
                    fprintf(stderr, "This will become fatal error in the future.\n");
                }
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
                if (optarg && isdigit(optarg[0]))
                {
                    *timeout = atoi(optarg);
                }
                else
                {
                    fprintf(stderr, "Option -W contains garbage as argument: %s.\n", optarg);
                    fprintf(stderr, "This will become fatal error in the future.\n");
                }
                break;
            case 'i':
                *flags |= I_FLAG;
                printf("%s\n", optarg);
                if (optarg && isdigit(optarg[0]))
                {
                    *interval = atof(optarg);
                }
                else
                {
                    fprintf(stderr, "Option -i contains garbage as argument: %s.\n", optarg);
                    fprintf(stderr, "This will become fatal error in the future.\n");
                }
                break;
            case 'D':
                *flags |= D_FLAG;
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
    double interval = 1;
    time_t timeout = 0;

    parse_argv(argc, argv, &flags, &destination, &preload, &timeout, &interval);

    ping(destination, flags, preload, timeout, interval);

    return 0;
}
