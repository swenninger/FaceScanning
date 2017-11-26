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

const int32_t COLOR_BUFFER_SIZE = NUM_COLOR_PIXELS * (int32_t)sizeof(RGB3f);
const int32_t DEPTH_BUFFER_SIZE = NUM_DEPTH_PIXELS * (int32_t)sizeof(uint8_t);

// TODO: See if this is enough, maybe switch to size_t?
const int32_t MAX_POINTCLOUD_SIZE = 262144;  // 2^18
const int32_t POINTCLOUD_BUFFER_SIZE = MAX_POINTCLOUD_SIZE * (int32_t)sizeof(Vec3f);

struct PointcloudBuffer {
    Vec3f points[MAX_POINTCLOUD_SIZE];
    RGB3f colors[MAX_POINTCLOUD_SIZE];
    Vec3f normals[MAX_POINTCLOUD_SIZE];

    size_t numPoints;
};

struct FrameBuffer {
    uint32_t colorBuffer[NUM_COLOR_PIXELS];
    uint8_t  depthBuffer[NUM_DEPTH_PIXELS];

    // TODO: See if this makes a difference
    // uint8_t  reserved;
    // uint16_t reserved;

    PointcloudBuffer pointcloudBuffer;
};

static void CopyFrameBuffer(FrameBuffer* src, FrameBuffer *dst) {
    memcpy(dst->colorBuffer, src->colorBuffer, COLOR_BUFFER_SIZE);
    memcpy(dst->depthBuffer, src->depthBuffer, DEPTH_BUFFER_SIZE);

    memcpy(dst->pointcloudBuffer.colors,  src->pointcloudBuffer.colors,  POINTCLOUD_BUFFER_SIZE);
    memcpy(dst->pointcloudBuffer.points,  src->pointcloudBuffer.points,  POINTCLOUD_BUFFER_SIZE);
    memcpy(dst->pointcloudBuffer.normals, src->pointcloudBuffer.normals, POINTCLOUD_BUFFER_SIZE);
    dst->pointcloudBuffer.numPoints = src->pointcloudBuffer.numPoints;
}

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

#endif // MEMORYPOOL_H
