#ifndef UTIL_H
#define UTIL_H

#include <stdint.h>
#include <string>
#include <iostream>
#include <fstream>


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
        B = *(data + 0) / 255.0f;
        G = *(data + 1) / 255.0f;
        R = *(data + 2) / 255.0f;
    }

    RGB3f (float r, float g, float b) {
        R = r;
        G = g;
        B = b;
    }

    float R, G, B;
};

struct PointCloud {
    Vec3f* points;
    RGB3f* colors;

    size_t size;

    /*
     * nanoflann interface functions
     */

    inline size_t kdtree_get_point_count() const { return size; }

    inline float kdtree_get_pt(const size_t idx, int dim) const {
        if (dim == 0)      { return points[idx].X; }
        else if (dim == 1) { return points[idx].Y; }
        else               { return points[idx].Z; }
    }

    template<class BBOX>
    bool kdtree_get_bbox(BBOX&) const { return false; }

};

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

static void LoadPointCloud(const std::string pointFile, const std::string colorFile, PointCloud* pc) {

    if (pc->colors) { delete pc->colors; }
    if (pc->points) { delete pc->points; }

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
    while (colors >> r >> g >> b) {
        RGB3f* c = colorData + count++;

        c->B = b;
        c->G = g;
        c->R = r;
    }
    colors.close();

}


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



#endif // UTIL_H
