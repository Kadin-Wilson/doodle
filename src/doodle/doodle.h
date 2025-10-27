#ifndef DOODLE_H
#define DOODLE_H

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include "point.h"

typedef enum {
    DOODLE_FT_PPM,
    DOODLE_FT_PNG,
} doodle_file_type;

typedef struct doodle_image doodle_image;

typedef struct {
    uint8_t r, g, b, a;
} doodle_color;

typedef struct {
    doodle_color background;
    uint32_t width;
    uint32_t height;
    doodle_file_type ft;
} doodle_config;

doodle_image *doodle_new(doodle_config *conf);

void doodle_draw_rect(
    doodle_image *img, 
    doodle_point orig, 
    uint32_t width, 
    uint32_t height,
    doodle_color color
);

void doodle_draw_circle(
    doodle_image *img,
    doodle_point orig,
    uint32_t radius,
    doodle_color color
);

void doodle_draw_line(
    doodle_image *img,
    doodle_point p1,
    doodle_point p2,
    double thickness,
    doodle_color color
);

bool doodle_export_ppm(doodle_image *img, FILE *out);
bool doodle_export_png(doodle_image *img, FILE *out);

bool doodle_export(doodle_image *img, doodle_config *conf, FILE *out);

#endif
