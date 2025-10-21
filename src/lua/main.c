#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "lua.h"
#include "doodle/doodle.h"

int main(int argc, char **argv) {
    FILE *in;

    switch (argc) {
    case 1:
        in = stdin;
        break;
    case 2:
        in = fopen(argv[1], "r");
        if (in == NULL) {
            fprintf(stderr, "failed to open %s: %s\n", argv[1], strerror(errno));
            return EXIT_FAILURE;
        }
        break;
    default:
        fputs("invalid number of arguements\n", stderr);
        return EXIT_FAILURE;
    }

    doodle_image *img = run_file(in);
    if (img == NULL) {
        fputs("failed to create image\n", stderr);
        return EXIT_FAILURE;
    }

    doodle_export_ppm(img, stdout);

    free(img);
    fclose(in);

    return EXIT_SUCCESS;
}
