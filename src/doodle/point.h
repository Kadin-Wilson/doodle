#ifndef DOODLE_POINT_H
#define DOODLE_POINT_H

#include <stdint.h>

typedef struct {
    double x, y;
} doodle_point;

doodle_point doodle_point_add(doodle_point p1, doodle_point p2);
doodle_point doodle_point_subtract(doodle_point p1, doodle_point p2);
doodle_point doodle_point_scale(doodle_point p1, double factor);
doodle_point doodle_point_polar_offset(
    doodle_point p1, 
    double radians, 
    double offset
);

#endif
