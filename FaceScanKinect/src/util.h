#ifndef UTIL_H
#define UTIL_H

#include <math.h>
#include <stdint.h>
#include <string>
#include <iostream>
#include <fstream>

#include <QtDebug>
#include <QtMath>
#include <QtGlobal>

#include "Types.h"

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

#include <random>

static inline float RandomFloat01() {
    float result = ((float)rand()) / RAND_MAX;

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

/**
 * @brief GenerateRandomHemiSphere Randomly generates points on a hemisphere (leaving out half the z-values) with the same method as GenerateRandomSphere()
 * @param numPoints
 * @param center
 * @param radius
 * @return
 */
static PointCloud GenerateRandomHemiSphere(int numPoints, Vec3f center = Vec3f(0.0f, 0.0f, 1.0f), float radius = 0.1f) {
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

        // discard vertices if they lie on the wrong hemisphere
        if (z > -0.09) {
            if (i > 0) {
                --i;
                continue;
            }
        }

        result.points[i] = Vec3f(center.X + radius * x,
                                 center.Y + radius * y,
                                 center.Z + radius * z);
        result.colors[i] = RGB3f(0.4f, 0.8f, 0.2f);
    }


    return result;
}


#include <Core>
#include <Eigenvalues>
#include "nanoflann.hpp"

typedef nanoflann::KDTreeSingleIndexAdaptor<
        nanoflann::L2_Simple_Adaptor<float, PointCloud>,
        PointCloud,
        3> KDTree;


static void FilterPointCloud(PointCloud in, PointCloud* out, int numNeighbors = 10, float stddevMultiplier = 1.0f) {
    float* distances = new float[in.size];

    KDTree tree(3, in, nanoflann::KDTreeSingleIndexAdaptorParams());
    tree.buildIndex();

    // Compute mean distance to k nearest neighbors for all points

    // TODO: How many neighbors?
    const int NUM_NEIGHBORS = numNeighbors;

    size_t numResults = NUM_NEIGHBORS;
    std::vector<size_t> indices(numResults);
    std::vector<float>  squaredDistances(numResults);

    for (size_t pointIndex = 0; pointIndex < in.size; pointIndex++) {
        numResults = NUM_NEIGHBORS;

        float distance = 0.0f;

        float* queryPoint = &(in.points[pointIndex].X);

        tree.knnSearch(queryPoint, numResults, &indices[0], &squaredDistances[0]);

        for (size_t neighbor = 0; neighbor < numResults; ++neighbor) {

            // TODO: check if squared distances are also ok!

            distance += sqrt(squaredDistances[neighbor]);
        }

        distance /= numResults;

        distances[pointIndex] = distance;
    }

    // Compute mean and stddev of all average distances

    float mean = 0.0f, stddev = 0.0f;
    for (size_t pointIndex = 0; pointIndex < in.size; pointIndex++) {
        float n = (float)pointIndex + 1.0f;

        float dist = distances[pointIndex];

        float previousMean = mean;
        mean   += (dist - mean) / n;
        stddev += (dist - mean) * (dist - previousMean);
    }
    stddev = sqrt(stddev / (float)in.size);

    // "Remove" points that are further than stddev_multitplier stddevs away from the mean

    // TODO: How many stddevs away do we allow?
    const float STDDEV_MULTIPLIER = stddevMultiplier;

    size_t numPointsInFilteredPointcloud = 0;
    for (size_t pointIndex = 0; pointIndex < in.size; pointIndex++) {
        if (distances[pointIndex] < mean + STDDEV_MULTIPLIER * stddev) {
            out->points[numPointsInFilteredPointcloud] = in.points[pointIndex];
            out->colors[numPointsInFilteredPointcloud] = in.colors[pointIndex];

            numPointsInFilteredPointcloud++;
        }
    }

    out->size = numPointsInFilteredPointcloud;

    delete [] distances;
    qInfo("Pointcloud filtered");
}


/**
 * Implements/Uses the aproach discussed in
 *      http://pointclouds.org/documentation/tutorials/normal_estimation.php
 */
static void ComputeNormalsForSnapshot(PointCloud in, Vec3f* out_normals)
{
    KDTree tree(3, in, nanoflann::KDTreeSingleIndexAdaptorParams());
    tree.buildIndex();

    // TODO: remove debug output
    // TODO: How many neighbors?
    size_t numResults = 15;
    std::vector<size_t> indices(numResults);
    std::vector<float>  squaredDistances(numResults);

    Eigen::Vector3f zero(0.0f, 0.0f, 0.0f);

    // For each point ...
    for (size_t pointIndex = 0; pointIndex < in.size; ++pointIndex) {
        numResults = 15;

        float* queryPoint = &(in.points[pointIndex].X);

        // ... get nearest neighbors ...
        numResults = tree.knnSearch(queryPoint, numResults, &indices[0], &squaredDistances[0]);

        // ... calculate Covariance Matrix ...
        Eigen::Vector3f centroid(0.0, 0.0, 0.0);
        for (size_t neighbor = 0; neighbor < numResults; ++neighbor) {
            centroid += Eigen::Map<Eigen::Vector3f>(&in.points[indices[neighbor]].X);
        }
        centroid /= numResults;

        Eigen::Matrix3f covarianceMatrix = Eigen::Matrix3f::Zero();
        for (size_t neighbor = 0; neighbor < numResults; ++neighbor) {
            Eigen::Vector3f d = Eigen::Map<Eigen::Vector3f>(&in.points[indices[neighbor]].X) - centroid;
            covarianceMatrix += d * d.transpose();
        }
        covarianceMatrix /= numResults;

        // ... Compute eigenvalues and eigenvectors of the covariance matrix. This gives us
        //     3 vectors that span a plane through the points ...

        // Eigenvalues are not sorted
        Eigen::EigenSolver<Eigen::Matrix3f> solver(covarianceMatrix, true);

        // ... smallest eigenvalue corresponds to the normal vector of the plane ...
        Eigen::Vector3f eigenValues = solver.eigenvalues().real();
        int min;
        eigenValues.minCoeff(&min);

        // TODO: simplify
        auto eigenvectorsComplex = solver.eigenvectors();
        Eigen::MatrixXf eigenvectorsReal = eigenvectorsComplex.real();
        Eigen::Vector3f normal = eigenvectorsReal.col(min);

        // ... 3D Sensor can only retrieve elements, that point towards it ...
        Eigen::Vector3f pointToView = zero - Eigen::Map<Eigen::Vector3f>(queryPoint);
        float dotProduct = normal.transpose() * pointToView;

        // ... flip normal if it does not point towards sensor ...
        if (dotProduct < 0) { normal = -normal; }

        out_normals[pointIndex] = Vec3f(normal.data()); // [0], normal[1], normal[2]);
    }
}

static void SaveSnaphot(PointCloud in, Vec3f* in_normals) {
    std::ofstream resultFile;
    resultFile.open("..\\..\\..\\data\\snapshot.pc");

    if (!resultFile.is_open()) {
        return;
    }

    for (size_t i = 0; i < in.size; ++i) {
        auto& point  = in.points[i];
        auto& color  = in.colors[i];
        auto& normal = in_normals[i];

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
