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

    pixmap.save("snapshot_color.bmp", "BMP");
}

static void SaveDepthImage(uint8_t* depth) {
    QPixmap pixmap = QPixmap::fromImage(QImage((uchar*)depth,
                                               DEPTH_WIDTH,
                                               DEPTH_HEIGHT,
                                               QImage::Format_Grayscale8));
    pixmap.save("snapshot_depth.bmp" ,"BMP");
}

static void LoadDepthImage() {

}

#endif // UTIL_H
