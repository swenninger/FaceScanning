#ifndef UTIL_H
#define UTIL_H

#include <random>
#include <stdint.h>
#include <string>
#include <iostream>
#include <fstream>

#include "Types.h"

static inline float RandomFloat01() {
    float result = ((float)rand()) / RAND_MAX;

    return result;
}

//
// LinearIndex Allows indexing 2D-data that has been stored in a flat buffer
//
#define LINEAR_INDEX(row,col,width) ((row) * (width) + (col))
static int LinearIndex(int row, int col, int width) {
    int result = row * width + col;
    return result;
}

static uint8_t SafeTruncateTo8Bit(int32_t val) {
    uint8_t result = val;
    if (val > 255) {
        result = 255;
    } else if (val < 0) {
        result = 0;
    }
    return result;
}

static uint8_t FloatToUINT8(float val) {
    // Round to integer
    int32_t iVal = (int32_t)floorf(val);
    // 0 to 255
    return SafeTruncateTo8Bit(iVal);
}


static void SavePointCloud(Vec3f* points, RGB3f* colors, Vec3f* normals, size_t numPoints) {
    std::ofstream resultFile;
    resultFile.open("..\\..\\data\\snapshot.pc");

    if (!resultFile.is_open()) {
        return;
    }

    for (size_t i = 0; i < numPoints; ++i) {
        auto& point  = points[i];
        auto& color  = colors[i];
        auto& normal = normals[i];

        resultFile << point.X  << " "
                   << point.Y  << " "
                   << point.Z  << " "
                   << (int )(color.R * 255.0f) << " "
                   << (int )(color.G * 255.0f) << " "
                   << (int )(color.B * 255.0f) << " "
                   << normal.X << " "
                   << normal.Y << " "
                   << normal.Z
                   << std::endl;
    }

    resultFile.close();
}

#include <QPixmap>
#include "MemoryPool.h"

static void SaveColorImage(uint32_t* colors) {
    QPixmap pixmap = QPixmap::fromImage(QImage((uchar*)colors,
                                               COLOR_WIDTH,
                                               COLOR_HEIGHT,
                                               QImage::Format_RGBA8888));

    pixmap.save("..\\..\\data\\snapshot_color.bmp", "BMP");
}

static void SaveDepthImage(uint8_t* depth) {
    QPixmap pixmap = QPixmap::fromImage(QImage((uchar*)depth,
                                               DEPTH_WIDTH,
                                               DEPTH_HEIGHT,
                                               QImage::Format_Grayscale8));
    pixmap.save("..\\..\\data\\snapshot_depth.bmp" ,"BMP");
}

static void LoadDepthImage() {

}
static void SaveLandmarks(size_t* landmarks, int numLandmarks) {
    std::ofstream resultFile;
    resultFile.open("..\\..\\data\\snapshot_indices.txt");

    for(int i = 0; i < numLandmarks; ++i) {
        resultFile << landmarks[i] << std::endl;
    }

    resultFile.close();
}

// Assumes, that landmarks has allocated enough space
static void LoadLandmarks(size_t* landmarks, int* numLandmarks) {
    std::ifstream resultFile;
    resultFile.open("..\\..\\data\\snapshot_indices.txt");

    int numLoadedLandmarks = 0;
    size_t landmarkIndex;
    while (resultFile >> landmarkIndex) {
        landmarks[numLoadedLandmarks++] = landmarkIndex;
    }

    Q_ASSERT(numLoadedLandmarks <= NUM_LANDMARKS);
    *numLandmarks = numLoadedLandmarks;
}



#endif // UTIL_H
