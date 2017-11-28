#ifndef TYPES_H
#define TYPES_H

#include <cstdint>

struct Vec3f {
    Vec3f () {}

    Vec3f (float* data) {
        X = *(data + 0);
        Y = *(data + 1);
        Z = *(data + 2);
    }

    Vec3f (float x, float y, float z) {
        X = x;
        Y = y;
        Z = z;
    }

    float X, Y, Z;
};

struct RGB3f {
    RGB3f () {}

    RGB3f (uint8_t* data) {
        B = *(data + 0) / 255.0f;
        G = *(data + 1) / 255.0f;
        R = *(data + 2) / 255.0f;
    }

    RGB3f (float r, float g, float b) {
        R = r;
        G = g;
        B = b;
    }

    RGB3f (unsigned char b, unsigned char g, unsigned char r) {
        R = ((float) r) / 255.0f;
        G = ((float) g) / 255.0f;
        B = ((float) b) / 255.0f;
    }

    float R, G, B;
};

#if 0
struct PointCloud {
    Vec3f* points;
    RGB3f* colors;

    size_t size;

    //
    // nanoflann interface functions
    //

    inline size_t kdtree_get_point_count() const { return size; }

    inline float kdtree_get_pt(const size_t idx, size_t dim) const {
        if      (dim == 0) { return points[idx].X; }
        else if (dim == 1) { return points[idx].Y; }
        else               { return points[idx].Z; }
    }

    template<class BBOX>
    bool kdtree_get_bbox(BBOX&) const { return false; }

};
#endif


#endif // TYPES_H
