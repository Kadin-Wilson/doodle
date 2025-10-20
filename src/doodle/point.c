#include <math.h>

#include "point.h"

doodle_point doodle_point_add(doodle_point p1, doodle_point p2) {
    return (doodle_point) {
        .x = p1.x + p2.x,
        .y = p1.y + p2.y
    };
}

doodle_point doodle_point_subtract(doodle_point p1, doodle_point p2) {
    return (doodle_point) {
        .x = p1.x - p2.x,
        .y = p1.y - p2.y
    };
}

doodle_point doodle_point_scale(doodle_point p1, double factor) {
    return (doodle_point) {
        .x = p1.x * factor,
        .y = p1.y * factor
    };
}

doodle_point doodle_point_polar_offset(
    doodle_point p1, 
    double radians, 
    double offset
) {
    return (doodle_point) {
        .x = offset * cos(radians) + p1.x,
        .y = offset * sin(radians) + p1.y
    };
}
