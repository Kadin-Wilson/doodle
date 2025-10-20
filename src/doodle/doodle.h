#ifndef DOODLE_H
#define DOODLE_H

#include <stdint.h>
#include <stdio.h>

#include "point.h"

typedef struct doodle_image doodle_image;

typedef struct {
    uint8_t r, g, b, a;
} doodle_color;

doodle_image *doodle_new(uint32_t width, uint32_t height, doodle_color background);

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

void doodle_export_ppm(doodle_image *img, FILE *out);

#endif
