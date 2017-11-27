#ifndef UTIL_H
#define UTIL_H

#include <math.h>
#include <random>
#include <stdint.h>
#include <string>
#include <iostream>
#include <fstream>

#include <QtDebug>
#include <QtMath>
#include <QtGlobal>

#include "Types.h"

static inline float RandomFloat01() {
    float result = ((float)rand()) / RAND_MAX;

    return result;
}

static void CopyPointCloud(PointCloud src, PointCloud* dst) {
    dst->size = src.size;

    if (dst->colors) { delete [] dst->colors; }
    if (dst->points) { delete [] dst->points; }

    dst->colors = new RGB3f[src.size];
    dst->points = new Vec3f[src.size];

    memcpy(dst->colors, src.colors, src.size * sizeof(RGB3f));
    memcpy(dst->points, src.points, src.size * sizeof(Vec3f));

    return;
}

#if 0
// TODO: DELETE!

static void LoadPointCloudFromFile(const char* pointFile, const char* colorFile, PointCloud* result) {
    std::ifstream file;
    file.open(colorFile);

    if (!file.is_open()) {
        std::cout << "Error: " << strerror(errno) << std::endl;

        return;
    }

    std::string line;
    std::vector<RGB3f> colors;
    while (std::getline(file, line)) {
        RGB3f color;
        const char* c = line.c_str();

        int pos = 1;
        c++; // skip '('

        int numDigits = 0;
        while (*c != ',') {
            ++numDigits;
            c++;
        }
        int b = atoi(line.substr(pos, pos + numDigits).c_str());
        color.B = b / 255.0f;

        c++; // Skip ','
        pos += numDigits + 1;

        numDigits = 0;
        while (*c != ',') {
            ++numDigits;
            c++;
        }
        double g = atoi(line.substr(pos, pos + numDigits).c_str());
        color.G = g / 255.0f;

        c++; // Skip ','

        pos += numDigits + 1;
        double r = atof(line.substr(pos, line.size() - 1 - pos).c_str());
        color.R = r/255.0f;

        colors.push_back(color);
    }

    file.close();

    int count = (int)colors.size();
    result->colors = new RGB3f[count];
    result->points = new Vec3f[count];
    result->size   = count;

    memcpy(result->colors, colors.data(), colors.size() * sizeof(Vec3f));

    file.open(pointFile);
    int index = 0;
    while (std::getline(file, line)) {
        Vec3f p;
        int pos = 0;
        const char* c = line.c_str();

        int numDigits = 0;
        while (*c != ',') {
            ++numDigits;
            c++;
        }
        double x = atof(line.substr(pos, pos + numDigits).c_str());
        p.X = (float)x;


        c++; // Skip ','

        pos += numDigits + 1;
        numDigits = 0;
        while (*c != ',') {
            ++numDigits;
            c++;
        }
        double y = atof(line.substr(pos, pos + numDigits).c_str());
        p.Y = (float)y;

        c++; // Skip ','

        pos += numDigits + 1;
        double z = atof(line.substr(pos, line.size() - pos).c_str());
        p.Z = (float)z;

        result->points[index++] = p;
    }

    file.close();
}
#endif

/**
 * @brief LoadPointCloud assumes two files in the following format:
 *
 * x1 y1 z1     (float)
 * x2 y2 z2
 *
 *    or
 *
 * b1 g1 r1     (float in range [0.0 - 1.0])
 * b2 g2 r2
 *
 * where x1 is either the x position of the 3D point or b1 is the blue value of the Color point
 *
 * @param pointFile
 * @param colorFile
 * @param pc
 */
static void LoadPointCloud(const std::string pointFile, const std::string colorFile, PointCloud* pc) {

    if (pc->colors) { delete [] pc->colors; }
    if (pc->points) { delete [] pc->points; }

    std::ifstream points;
    points.open(pointFile);

    std::ifstream colors;
    colors.open(colorFile);

    if (!points.is_open() ||
        !colors.is_open()) {
        return;
    }

    std::string line;

    int count = 0;
    while (std::getline(points, line)) {
        ++count;
    }
    points.close();

    Vec3f* pointData = new Vec3f[count];
    RGB3f* colorData = new RGB3f[count];

    pc->size = count;
    pc->colors = colorData;
    pc->points = pointData;

    points.open(pointFile);

    float x, y, z;
    count = 0;
    while (points >> x >> y >> z) {
        Vec3f* p = pointData + count++;

        p->X = x;
        p->Y = y;
        p->Z = z;
    }
    points.close();

    count = 0;
    float b, g, r;
    while (colors >> b >> g >> r) {
        RGB3f* c = colorData + count++;

        c->B = b;
        c->G = g;
        c->R = r;
    }
    colors.close();
}

/**
 * @brief WritePointCloudToFile Writes pointcloud data to two files, in the format specified in LoadPointCloud
 *
 * @param pointFile
 * @param colorFile
 * @param out
 */
static void WritePointCloudToFile(const char* pointFile, const char* colorFile, PointCloud out) {
    std::ofstream resultPoints;
    resultPoints.open(pointFile);

    std::ofstream resultColors;
    resultColors.open(colorFile);

    if (!resultColors.is_open() ||
        !resultPoints.is_open()) {

        return;
    }

    for (int i = 0; i < out.size; ++i) {
        auto& point = out.points[i];

        resultPoints << point.X << " "
                     << point.Y << " "
                     << point.Z << std::endl;

        auto& color = out.colors[i];
        resultColors << color.B << " "
                     << color.G << " "
                     << color.R << std::endl;
    }

    resultColors.close();
    resultPoints.close();

}

/**
 * @brief GenerateSphere try to generate points on a sphere by uniformly stepping through polar coordinates. Does not return full sphere as of now!
 * @param center
 * @param radius
 * @param resolution
 * @return
 */
static PointCloud GenerateSphere(Vec3f center = Vec3f(0.0f, 0.3f, 1.2f), float radius = 0.1f, int resolution = 50) {
    PointCloud result = {};

    int vResolution = resolution;
    int uResolution = 2 * resolution;

    int numPoints = (int) (vResolution * uResolution);
    result.points = new Vec3f[numPoints];
    result.colors = new RGB3f[numPoints];
    result.size   = numPoints;

    int counter = 0;
    qInfo("Sphere initialized");

    float x,y,z, u, v, theta, phi;

    for (int iv = 0; iv < vResolution; iv++) {
        for (int iu = 0; iu < uResolution; iu++) {

            v = (float)iv / (float)vResolution;
            u = (float)iu / (float)uResolution;

            phi   = v * M_PI;
            theta = u * 2.0f * M_PI;

            x = cos(theta) * sin(phi);
            y = sin(theta) * sin(phi);
            z = cos(phi);
            // x = cos(phi);
            // y = cos(theta) * sin(phi);


            result.points[counter] = Vec3f(center.X + (radius * x),
                                           center.Y + (radius * y),
                                           center.Z + (radius * z));

            result.colors[counter] = RGB3f(0.0f, 1.0f, 0.3f);

            ++counter;
        }
    }

    qInfo() << counter;
    qInfo() << numPoints;
    qInfo("Sphere generated");

    Q_ASSERT(numPoints == counter);

    return result;
}



/**
 * @brief GenerateRandomSphere Randomly generates numPoints points on a sphere by uniformly sampling polar coordinates
 * @param numPoints
 * @param center
 * @param radius
 * @return
 */
static PointCloud GenerateRandomSphere(int numPoints, Vec3f center = Vec3f(0.0f, 0.0f, 1.0f), float radius = 0.1f) {
    PointCloud result = {};

    result.size = numPoints;
    result.points = new Vec3f[numPoints];
    result.colors = new RGB3f[numPoints];

    for (int i = 0; i < numPoints; ++i) {

        float theta = 2 * M_PI * RandomFloat01();
        float phi   = acos(1 - 2 * RandomFloat01());

        double x = sin(phi) * cos(theta);
        double y = sin(phi) * sin(theta);
        double z = cos(phi);

        result.points[i] = Vec3f(center.X + radius * x,
                                 center.Y + radius * y,
                                 center.Z + radius * z);
        result.colors[i] = RGB3f(0.4f, 0.8f, 0.2f);
    }


    return result;
}


static void SaveSnaphot(Vec3f* points, RGB3f* colors, Vec3f* normals, size_t numPoints) {
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

static void LoadSnapshot(const std::string pointcloudFilename, PointCloud* pc, Vec3f** normals) {

    if (pc->colors) { delete [] pc->colors; }
    if (pc->points) { delete [] pc->points; }
    if (normals && *normals)    { delete [] (*normals); }

    std::ifstream pointcloudFile;
    pointcloudFile.open(pointcloudFilename);

    if (!pointcloudFile.is_open()) {
        return;
    }

    std::string line;

    int count = 0;
    while (std::getline(pointcloudFile, line)) {
        ++count;
    }
    pointcloudFile.close();

    Vec3f* pointData = new Vec3f[count];
    RGB3f* colorData = new RGB3f[count];
    Vec3f* normalData = new Vec3f[count];

    pc->size = count;
    pc->colors = colorData;
    pc->points = pointData;
    *normals = normalData;

    pointcloudFile.open(pointcloudFilename);

    float x, y, z;
    int   r, g, b;
    float nx, ny, nz;

    count = 0;
    while (pointcloudFile >> x  >> y  >> z
                          >> r  >> g  >> b
                          >> nx >> ny >> nz) {
        Vec3f* p = pointData  + count;
        RGB3f* c = colorData  + count;
        Vec3f* n = (*normals) + count;

        p->X = x;
        p->Y = y;
        p->Z = z;

        c->R = (float)(r / 255.0f);
        c->G = (float)(g / 255.0f);
        c->B = (float)(b / 255.0f);

        n->X = nx;
        n->Y = ny;
        n->Z = nz;

        count++;
    }
    pointcloudFile.close();
}



#endif // UTIL_H
