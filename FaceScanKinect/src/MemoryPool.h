#ifndef MEMORYPOOL_H
#define MEMORYPOOL_H

#include <cstdint>
#include <cstring>

#include "Types.h"

const int32_t COLOR_WIDTH  = 1920;
const int32_t COLOR_HEIGHT = 1080;
const int32_t DEPTH_WIDTH  =  512;
const int32_t DEPTH_HEIGHT =  424;

const int32_t NUM_COLOR_PIXELS = COLOR_WIDTH * COLOR_HEIGHT;
const int32_t NUM_DEPTH_PIXELS = DEPTH_WIDTH * DEPTH_HEIGHT;

const int32_t COLOR_BUFFER_SIZE = NUM_COLOR_PIXELS * (int32_t)sizeof(uint32_t);
const int32_t DEPTH_BUFFER8_SIZE = NUM_DEPTH_PIXELS * (int32_t)sizeof(uint8_t);
const int32_t DEPTH_BUFFER16_SIZE = NUM_DEPTH_PIXELS * (int32_t)sizeof(uint16_t);

// TODO: See if this is enough, maybe switch to size_t?
const int32_t MAX_POINTCLOUD_SIZE = 262144;  // 2^18
const int32_t POINTCLOUD_BUFFER_SIZE = MAX_POINTCLOUD_SIZE * (int32_t)sizeof(Vec3f);

const int NUM_LANDMARKS = 68;
const int LANDMARK_BUFFER_SIZE = NUM_LANDMARKS * sizeof(size_t);

struct PointCloudBuffer {

    PointCloudBuffer() {
        points = new Vec3f[MAX_POINTCLOUD_SIZE];
        colors = new RGB3f[MAX_POINTCLOUD_SIZE];
        normals = new Vec3f[MAX_POINTCLOUD_SIZE];
        landmarkIndices = new size_t[NUM_LANDMARKS];
        numLandmarks = 0;
        numPoints = 0;
    }

    ~PointCloudBuffer() {
        delete [] points;
        delete [] colors;
        delete [] normals;
    }

    Vec3f* points;
    RGB3f* colors;
    Vec3f* normals;

    size_t numPoints;

    size_t* landmarkIndices;
    int numLandmarks;

    //
    // nanoflann interface functions
    //

    inline size_t kdtree_get_point_count() const { return numPoints; }

    inline float kdtree_get_pt(const size_t idx, size_t dim) const {
        if      (dim == 0) { return points[idx].X; }
        else if (dim == 1) { return points[idx].Y; }
        else               { return points[idx].Z; }
    }

    template<class BBOX>
    bool kdtree_get_bbox(BBOX&) const { return false; }


};

struct FrameBuffer {

    FrameBuffer() {
        colorBuffer = new uint32_t[NUM_COLOR_PIXELS];
        depthBuffer8 = new uint8_t[NUM_DEPTH_PIXELS];
        depthBuffer16 = new uint16_t[NUM_DEPTH_PIXELS];
        colorToCameraMapping = new Vec3f[NUM_COLOR_PIXELS];

        pointCloudBuffer = new PointCloudBuffer();
    }

    ~FrameBuffer() {
        delete [] colorBuffer;
        delete [] depthBuffer8;
        delete [] depthBuffer16;
        delete [] colorToCameraMapping;
    }

    // BGRA with 1Byte each
    uint32_t* colorBuffer;

    Vec3f*    colorToCameraMapping;

    uint16_t* depthBuffer16;
    uint8_t * depthBuffer8;

    // TODO: See if this padding makes a difference
    // uint8_t  reserved;
    // uint16_t reserved;

    PointCloudBuffer* pointCloudBuffer;
};

static void CopyPointCloudBuffer(PointCloudBuffer* src, PointCloudBuffer* dst) {
    memcpy(dst->colors,  src->colors,  POINTCLOUD_BUFFER_SIZE);
    memcpy(dst->points,  src->points,  POINTCLOUD_BUFFER_SIZE);
    memcpy(dst->normals, src->normals, POINTCLOUD_BUFFER_SIZE);
    memcpy(dst->landmarkIndices, src->landmarkIndices, LANDMARK_BUFFER_SIZE);
    dst->numLandmarks = src->numLandmarks;
    dst->numPoints = src->numPoints;
}

static void CopyFrameBuffer(FrameBuffer* src, FrameBuffer *dst) {
    memcpy(dst->colorBuffer, src->colorBuffer, COLOR_BUFFER_SIZE);
    memcpy(dst->depthBuffer8, src->depthBuffer8, DEPTH_BUFFER8_SIZE);
    memcpy(dst->depthBuffer16, src->depthBuffer16, DEPTH_BUFFER16_SIZE);
    CopyPointCloudBuffer(src->pointCloudBuffer, dst->pointCloudBuffer);
}

struct MemoryPool {
    FrameBuffer gatherBuffer;
    FrameBuffer snapshotBuffer;

    PointCloudBuffer inspectionBuffer;
    PointCloudBuffer filterBuffer;

    // FrameBuffer displayBuffer;
    // FrameBuffer inspectionBuffer;
};

#if 0
//
// Global Buffers
//
static FrameBuffer _gatherBuffer;
static FrameBuffer _displayBuffer;
static FrameBuffer _snapshotBuffer;
static FrameBuffer _inspectionBuffer;

static FrameBuffer* gatherBuffer     = &_gatherBuffer;
static FrameBuffer* displayBuffer    = &_displayBuffer;
static FrameBuffer* snapshotBuffer   = &_snapshotBuffer;
static FrameBuffer* inspectionBuffer = &_inspectionBuffer;
#endif

#endif // MEMORYPOOL_H
