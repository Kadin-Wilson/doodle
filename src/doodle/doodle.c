#include <stdbool.h>
#include <inttypes.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "doodle.h"

#define DELTA 0.0001
#define DIFF(a, b) fmax(fdim((a), (b)), fdim((b), (a)))

struct doodle_image {
    uint32_t width, height;
    doodle_color pixels[];
};

static doodle_color *get_pixel(doodle_image *img, uint32_t x, uint32_t y) {
    return img->pixels + ((size_t)y * img->width) + x;
}

doodle_image *doodle_new(uint32_t width, uint32_t height, doodle_color background) {
    size_t pixel_count = (size_t)width * height;

    doodle_image *img = malloc(pixel_count * sizeof *(img->pixels) + sizeof *img);
    if (img == NULL) {
        return NULL;
    }

    img->width = width;
    img->height = height;

    for (size_t i = 0; i < pixel_count; i++) {
        img->pixels[i] = background;
    }

    return img;
}

void doodle_draw_rect(
    doodle_image *img, 
    doodle_point orig, 
    uint32_t width, 
    uint32_t height,
    doodle_color color
) {
    if (orig.x < 0) {
        uint32_t move = -orig.x;
        width = width > move ? width - move : 0;
        orig.x = 0;
    }
    if (orig.y < 0) {
        uint32_t move = -orig.y;
        height = height > move ? height - move : 0;
        orig.y = 0;
    }

    for (uint32_t x = orig.x; x < img->width && x < width; x++) {
        for (uint32_t y = orig.y; y < img->height && y < height; y++) {
            *get_pixel(img, x, y) = color;
        }
    }
}

void doodle_draw_circle(
    doodle_image *img,
    doodle_point orig,
    uint32_t radius,
    doodle_color color
) {
    int64_t srad = radius;

    // if circle doesn't overlap with image do nothing
    if (orig.x < 0 && orig.x + srad < 0) return;
    if (orig.y < 0 && orig.y + srad < 0) return;
    if (orig.y > 0 && orig.y - srad >= (int64_t)img->height) return;
    if (orig.x > 0 && orig.x - srad >= (int64_t)img->width) return;

    // don't start before image
    uint32_t startx = orig.x > 0 && orig.x > radius ? orig.x - radius : 0;
    uint32_t starty = orig.y > 0 && orig.y > radius ? orig.y - radius : 0;

    // don't go past end of image
    uint32_t endx = orig.x + srad;
    uint32_t endy = orig.y + srad;
    endx = endx < img->width ? endx : img->width - 1;
    endy = endy < img->height ? endy : img->height - 1;

    for (uint32_t x = startx; x <= endx; x++) {
        for (uint32_t y = starty; y <= endy; y++) {
            double h = hypot(DIFF(x, orig.x), DIFF(y, orig.y));
            if (DIFF(h, radius) <= DELTA) {
                *get_pixel(img, x, y) = color;
            }
        }
    }
}

void doodle_export_ppm(doodle_image *img, FILE *out) {
    fprintf(out, "P6\n%"PRId32" %"PRId32"\n255\n", img->width, img->height);

    char buf[3];
    for (size_t i = 0; i < img->width * img->height; i++) {
        buf[0] = img->pixels[i].r;
        buf[1] = img->pixels[i].g;
        buf[2] = img->pixels[i].b;
        fwrite(buf, 1, 3, out);
    }
}
