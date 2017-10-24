#ifndef UTIL_H
#define UTIL_H

#include <stdint.h>

struct Vec3f {
    Vec3f () {}

    Vec3f (float* data) {
        X = *(data + 0);
        Y = *(data + 1);
        Z = *(data + 2);
    }

    float X, Y, Z;
};

struct RGB3f {
    RGB3f () {}

    RGB3f (uint8_t* data) {
        R = *(data + 0) / 255.0f;
        G = *(data + 1) / 255.0f;
        B = *(data + 2) / 255.0f;
    }

    float R, G, B;
};

#endif // UTIL_H
