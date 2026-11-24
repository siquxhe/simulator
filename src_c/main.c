#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <getopt.h>
#include <errno.h>
#include <err.h>
#include <ctype.h>

#include "tape.h"
#include "103.h"

int flag_decimal = 0;

static char short_options[] = "s:dDS";
static struct option long_options[] = {
    {"start",   required_argument,  0,  's'},
    {"decimal", no_argument,        0,  'd'},
    {"input-decimal", no_argument,  0,  'D'},
    {"stop-after-output", no_argument, 0, 'S'},
    {0,         0,                  0,   0 },
};

int main(int argc, char ** argv) {
    set_output_numeral(OCTAL);
    set_input_numeral(OCTAL);
    set_stop_after_output(0);
    set_pc(0);

    while (1) {
        int c = getopt_long(argc, argv, short_options, long_options, NULL);
        if (c == -1) {
            break;
        }

        if (c == 0) {
            // nothing to do
        } else if (c == 's') {
            int start_addr;
            if (sscanf(optarg, "%o", &start_addr) != 1) {
                err(EXIT_FAILURE, "%s", optarg);
            }
            set_pc(start_addr);
        } else if (c == 'd') {
            set_output_numeral(DECIMAL);

        } else if (c == 'D') {
            set_input_numeral(DECIMAL);

        } else if (c == 'S') {
            set_stop_after_output(1);

        } else {
            printf("%c\n", c);
            fprintf(stderr, "Usage: %s [-s <start_addr>] [-d] <tape_file>\n", argv[0]);
            exit(EXIT_FAILURE);
        }
    }

    if (optind >= argc) {
        fprintf(stderr, "1Usage: %s [-s <start_addr>] [-d] <tape_file>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    input_file = fopen(argv[optind], "rb");
    if (!input_file) {
        err(EXIT_FAILURE, "%s", argv[optind]);
    }
    output_file = fopen("output.103", "w");
    
    init_mem();

    do_input(0);
    run_machine();

    // print_mem();

    exit(EXIT_SUCCESS);
}