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

    float R, G, B;
};

struct PointCloud {
    Vec3f* points;
    RGB3f* colors;

    int size;
};

static void LoadPointCloudFromFile(const char* pointFile, const char* colorFile, PointCloud* out) {
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
    out->colors = new RGB3f[count];
    out->points = new Vec3f[count];
    out->size   = count;

    memcpy(out->colors, colors.data(), colors.size() * sizeof(Vec3f));

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

        out->points[index++] = p;
    }

    file.close();

}


#endif // UTIL_H
