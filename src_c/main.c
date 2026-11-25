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

static const char *image_path = NULL;

static char short_options[] = "s:dDSm:";
static struct option long_options[] = {
    {"start",   required_argument,  0,  's'},
    {"decimal", no_argument,        0,  'd'},
    {"input-decimal", no_argument,  0,  'D'},
    {"stop-after-output", no_argument, 0, 'S'},
    {"image",   required_argument,  0,  'm'},
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

        } else if (c == 'm') {
            image_path = optarg;

        } else {
            printf("%c\n", c);
            fprintf(stderr, "Usage: %s [-s <start_addr>] [-d] <tape_file>\n", argv[0]);
            exit(EXIT_FAILURE);
        }
    }

    output_file = fopen("output.103", "w");

    init_mem();

    if (image_path) {
        /* P0-2 结构化镜像通路：校验 + 按段装载 + 置入口 PC；不走过纸带符号流。 */
        if (load_image(image_path) != 0) {
            exit(EXIT_FAILURE);
        }
    } else {
        if (optind >= argc) {
            fprintf(stderr, "Usage: %s [-s <start_addr>] [-d] <tape_file>\n", argv[0]);
            fprintf(stderr, "       %s -m <image.bin>\n", argv[0]);
            exit(EXIT_FAILURE);
        }
        input_file = fopen(argv[optind], "rb");
        if (!input_file) {
            err(EXIT_FAILURE, "%s", argv[optind]);
        }
        do_input(0);
    }
    run_machine();

    // print_mem();

    exit(EXIT_SUCCESS);
}