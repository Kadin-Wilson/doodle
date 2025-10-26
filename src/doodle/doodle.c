#include <stdbool.h>
#include <inttypes.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include <png.h>

#include "doodle.h"

#define DIFF(a, b) fmax(fdim((a), (b)), fdim((b), (a)))

#define PIXEL_SIZE 4

struct doodle_image {
    uint32_t width, height;
    uint8_t pixels[];
};

static void copy_color(uint8_t *dst, doodle_color c) {
    *dst++ = c.r;
    *dst++ = c.g;
    *dst++ = c.b;
    *dst = c.a;
}

static void set_pixel(doodle_image *img, uint32_t x, uint32_t y, doodle_color c) {
    copy_color(img->pixels + img->width * y * PIXEL_SIZE + x * PIXEL_SIZE, c);
}

doodle_image *doodle_new(doodle_config *conf) {
    size_t pixel_count = (size_t)conf->width * conf->height;

    doodle_image *img = malloc(pixel_count * PIXEL_SIZE + sizeof *img);
    if (img == NULL) {
        return NULL;
    }

    img->width = conf->width;
    img->height = conf->height;

    for (size_t i = 0; i < pixel_count; i++) {
        copy_color(img->pixels + i * PIXEL_SIZE, conf->background);
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

    for (uint32_t x = orig.x; x < img->width && x - orig.x <= width; x++) {
        for (uint32_t y = orig.y; y < img->height && y - orig.y <= height; y++) {
            set_pixel(img, x, y, color);
        }
    }
}

void doodle_draw_circle(
    doodle_image *img,
    doodle_point orig,
    uint32_t radius,
    doodle_color color
) {
    double drad = radius;

    // if circle doesn't overlap with image do nothing
    if (orig.x < 0 && orig.x + drad < 0) return;
    if (orig.y < 0 && orig.y + drad < 0) return;
    if (orig.y > 0 && orig.y - drad >= (int64_t)img->height) return;
    if (orig.x > 0 && orig.x - drad >= (int64_t)img->width) return;

    // don't start before image
    uint32_t startx = orig.x > 0 && orig.x > drad ? (double)orig.x - radius : 0;
    uint32_t starty = orig.y > 0 && orig.y > drad ? (double)orig.y - radius : 0;

    // don't go past end of image
    uint32_t endx = (double)orig.x + drad;
    uint32_t endy = (double)orig.y + drad;
    endx = endx < img->width ? endx : img->width - 1;
    endy = endy < img->height ? endy : img->height - 1;

    for (uint32_t x = startx; x <= endx; x++) {
        for (uint32_t y = starty; y <= endy; y++) {
            if (hypot(DIFF(x, orig.x), DIFF(y, orig.y)) < drad + 1) {
                set_pixel(img, x, y, color);
            }
        }
    }
}

void doodle_export(doodle_image *img, doodle_config *conf, FILE *out) {
    switch (conf->ft) {
    case DOODLE_FT_PPM:
        doodle_export_ppm(img, out);
        return;
    case DOODLE_FT_PNG:
        doodle_export_png(img, out);
        return;
    }
}

void doodle_export_ppm(doodle_image *img, FILE *out) {
    fprintf(out, "P6\n%"PRId32" %"PRId32"\n255\n", img->width, img->height);

    for (size_t i = 0; i < img->width * img->height; i ++) {
        fwrite(img->pixels + i * PIXEL_SIZE, 3, 1, out);
    }
}

void doodle_export_png(doodle_image *img, FILE *out) {
    uint8_t **pixel_rows = malloc(img->height * sizeof *pixel_rows);
    for (size_t i = 0; i < img->height; i++) {
        pixel_rows[i] = img->pixels + i * img->width * PIXEL_SIZE;
    }

    png_structp png_p = png_create_write_struct(
        PNG_LIBPNG_VER_STRING, NULL, NULL, NULL
    );
    if (png_p == NULL) {
        goto png_p_error;
    }

    png_infop info_p = png_create_info_struct(png_p);
    if (info_p == NULL) {
        goto info_p_error;
    }

    if (setjmp(png_jmpbuf(png_p))) {
        goto png_error;
    }

    png_init_io(png_p, out);

    png_set_IHDR(
        png_p, info_p,
        img->width, img->height, 
        8, PNG_COLOR_TYPE_RGBA,
        PNG_INTERLACE_NONE,
        PNG_COMPRESSION_TYPE_DEFAULT,
        PNG_FILTER_TYPE_DEFAULT
    );
    png_set_invert_alpha(png_p);

    png_write_info(png_p, info_p);
    png_write_image(png_p, pixel_rows);
    png_write_end(png_p, NULL);

    png_destroy_write_struct(&png_p, &info_p);

    return;

png_error:
    png_destroy_write_struct(NULL, &info_p);
info_p_error:
    png_destroy_write_struct(&png_p, NULL);
png_p_error:
    fputs("failed to create png image\n", stderr);
}
