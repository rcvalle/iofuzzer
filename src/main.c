/** @file */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "../lib/error.h"
#include "../lib/string.h"
#include "lib/io_fuzzer.h"

#include <errno.h>
#include <getopt.h>
#include <limits.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <sys/io.h>
#include <unistd.h>

#define MAX_PORTS 65536

#define usage() \
    fprintf(stderr, \
            "Usage: %s [OPTION]... [INPUT]\n" \
            "Options:\n" \
            "  -d, --debug           Enable debug mode.\n" \
            "  -g, --generate        Use the pseudorandom number generator (i.e., random())\n" \
            "                        for input generation.\n" \
            "  -h, --help            Display help information and exit.\n" \
            "  -o, --output=FILE     Specify the output file name.\n" \
            "  -p, --ports=LIST      Specify the list of I/O port addresses. (The default is\n" \
            "                        all ports.)\n" \
            "  -q, --quiet           Enable quiet mode.\n" \
            "  -s, --seed=NUM        Specify the seed for the pseudorandom number generator.\n" \
            "                        (The default is 1.)\n" \
            "  -t, --timeout=NUM     Specify the timeout, in seconds, for each iteration.\n" \
            "                        (The default is 5.)\n" \
            "  -v, --verbose         Enable verbose mode.\n" \
            "      --version         Display version information and exit.\n", \
            PACKAGE_NAME)

#define version() fprintf(stderr, "%s\n", PACKAGE_STRING)

void
default_error_handler(int status, int error, const char *restrict format, va_list ap)
{
    fflush(stdout);
    vfprintf(stderr, format, ap);
    if (error != 0) {
        fprintf(stderr, ": %s\n", strerror(error));
    }

    fflush(stderr);
    abort();
}

void
default_log_handler(FILE *restrict stream, const char *restrict format, va_list ap)
{
    flockfile(stream);
    fprintf(stream, "{ ");
    fprintf(stream, "\"time\": %d,", (unsigned int)time(NULL));
    for (size_t i = 0; format[i] != '\0'; ++i) {
        if (i > 0) {
            fprintf(stream, ", ");
        }

        fprintf(stream, "\"%s\": ", va_arg(ap, char *));
        switch (format[i]) {
        case 'c':
            fprintf(stream, "\"%c\"", va_arg(ap, int));
            break;

        case 'd':
            fprintf(stream, "%d", va_arg(ap, int));
            break;

        case 'f':
            fprintf(stream, "%f", va_arg(ap, double));
            break;

        case 'o':
            fprintf(stream, "%o", va_arg(ap, unsigned int));
            break;

        case 'p':
            fprintf(stream, "%p", va_arg(ap, void *));
            break;

        case 'q':
            fprintf(stream, "%llu", va_arg(ap, unsigned long long int));
            break;

        case 's':
            fprintf(stream, "\"%s\"", va_arg(ap, char *));
            break;

        case 'u':
            fprintf(stream, "%u", va_arg(ap, unsigned int));
            break;

        case 'x':
            fprintf(stream, "%x", va_arg(ap, unsigned int));
            break;

        case 'z':
            fprintf(stream, "%zu", va_arg(ap, size_t));
            break;

        default:
            abort();
        }
    }

    fprintf(stream, " }\n");
    fflush(stream);
    fsync(fileno(stream));
    funlockfile(stream);
}

void
random_buf(void *buf, size_t size)
{
    uint32_t number = 0;
    for (size_t i = 0; i < size; ++i) {
        if ((i % sizeof(uint16_t)) == 0) {
            number = random();
        }

        ((uint8_t *)buf)[i] = (number >> (8 * (i % sizeof(uint16_t)))) & 0xff;
    }
}

int
main(int argc, char *argv[])
{
    int c = 0;
    enum
    {
        OPT_VERSION = CHAR_MAX + 1,
    };
    /* clang-format off */
    static struct option longopts[] = {
        {"debug",       no_argument,       NULL, 'd'             },
        {"generate",    no_argument,       NULL, 'g'             },
        {"help",        no_argument,       NULL, 'h'             },
        {"output",      required_argument, NULL, 'o'             },
        {"ports",       required_argument, NULL, 'p'             },
        {"quiet",       no_argument,       NULL, 'q'             },
        {"seed",        required_argument, NULL, 's'             },
        {"timeout",     required_argument, NULL, 't'             },
        {"verbose",     no_argument,       NULL, 'v'             },
        {"version",     no_argument,       NULL, OPT_VERSION     },
        {NULL,          0,                 NULL, 0               }
    };
    /* clang-format on */
    static int longindex = 0;
    int debug = 0;
    int generate = 0;
    char *input = NULL;
    char *output = NULL;
    int *ports = NULL;
    size_t num_ports = 0;
    int quiet = 0;
    unsigned long seed = 1;
    int timeout = 5;
    int verbose = 0;
    while ((c = getopt_long(argc, argv, "dgho:p:qs:t:v", longopts, &longindex)) != -1) {
        switch (c) {
        case 'd':
            debug = 1;
            break;

        case 'g':
            generate = 1;
            break;

        case 'h':
            usage();
            exit(EXIT_FAILURE);

        case 'o':
            output = optarg;
            break;

        case 'p':
            if (string_split_range(optarg, ",", MAX_PORTS, &ports, &num_ports) == -1) {
                perror("getlist");
                exit(EXIT_FAILURE);
            }

            break;

        case 'q':
            quiet = 1;
            break;

        case 's':
            errno = 0;
            seed = strtoul(optarg, NULL, 0);
            if (errno != 0) {
                perror("strtoul");
                exit(EXIT_FAILURE);
            }

            break;

        case 't':
            errno = 0;
            timeout = strtoul(optarg, NULL, 0);
            if (errno != 0) {
                perror("strtoul");
                exit(EXIT_FAILURE);
            }

            break;

        case 'v':
            verbose = 1;
            break;

        case OPT_VERSION:
            version();
            exit(EXIT_FAILURE);

        default:
            usage();
            exit(EXIT_FAILURE);
        }
    }

    FILE *stream = stdout;
    if (output != NULL) {
        stream = fopen(output, "a+");
        if (stream == NULL) {
            perror("fopen");
            exit(EXIT_FAILURE);
        }
    }

    if (iopl(3) == -1) {
        perror("iopl");
        exit(EXIT_FAILURE);
    }

    io_fuzzer_set_error_handler(default_error_handler);
    io_fuzzer_t *io_fuzzer = io_fuzzer_create(ports, num_ports);
    if (io_fuzzer == NULL) {
        perror("io_fuzzer_create");
        goto err;
    }

    io_fuzzer_set_log_handler(io_fuzzer, default_log_handler);
    io_fuzzer_set_log_stream(io_fuzzer, stream);
    if (generate) {
        srandom(seed);
        for (;;) {
            uint8_t buf[IO_FUZZER_MAX_INPUT];
            random_buf(buf, sizeof(buf));
            FILE *stream = fmemopen(buf, sizeof(buf), "r");
            if (stream == NULL) {
                perror("fmemopen");
                goto err;
            }

            io_fuzzer_iterate(io_fuzzer, stream);
            fclose(stream);
        }
    } else {
        if (argv[optind] != NULL) {
            input = argv[optind];
        }

        FILE *stream = stdin;
        if (input != NULL) {
            FILE *stream = fopen(input, "r");
            if (stream == NULL) {
                perror("fopen");
                exit(EXIT_FAILURE);
            }
        }

        io_fuzzer_iterate(io_fuzzer, stream);
        fclose(stream);
    }

    io_fuzzer_destroy(io_fuzzer);
    fclose(stream);
    free(ports);
    exit(EXIT_SUCCESS);

err:
    io_fuzzer_destroy(io_fuzzer);
    fclose(stream);
    free(ports);
    exit(EXIT_FAILURE);
}
