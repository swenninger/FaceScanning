/* 
 * File:   PointProjector.cpp
 * Author: rzorn
 * 
 * Created on 26. Juli 2015, 14:49
 */

#include "PointProjector.h"

using namespace std;
using namespace cv;

const PointProjector PointProjector::ASUS_XTION_VGA_100_UM(58, 45, cv::Size(640, 480), 0.0001f);
const PointProjector PointProjector::ASUS_XTION_QVGA_100_UM(58, 45, cv::Size(320, 240), 0.0001f);
const PointProjector PointProjector::KINECT2(70, 60, cv::Size(512, 424), 0.0001f);

Point3f PointProjector::project2Dto3D(const Point2f point, const uint16_t value) const {
    Point3d result;

    float normalizedX = point.x / size.width - .5f;
    float normalizedY = .5f - point.y / size.height;

    result.x = normalizedX * value * xzfactor * toMeters;
    result.y = normalizedY * value * yzfactor * toMeters;
    result.z = value * toMeters;

    return result;
}

Point2f PointProjector::project3Dto2D(const Point3f point3d, uint16_t* value) const {
    Point2f result;

    Point3f normalized = point3d * (1.0 / toMeters);
    
    result.x = coeffX * normalized.x / normalized.z + size.width / 2;
    result.y = size.height / 2 - coeffY * normalized.y / normalized.z;
    
    if(value != NULL) {
        *value = uint16_t(normalized.z);
    }

    return result;
}

std::vector<cv::Point3f> PointProjector::get3DPoints(const std::vector<cv::Point2i>& points2d, const cv::Mat& depthimage) const {
    vector<Point3f> points3d;
    
    points3d.reserve(points2d.size());
    for (int i = 0; i < points2d.size(); i++) {
        Point2i point2d = points2d[i];
        uint16_t value = ((uint16_t*) (depthimage.ptr(point2d.y)))[point2d.x];

        if (value > 0) {
            Point3f p = project2Dto3D(point2d, value);
            points3d.push_back(p);
        }
    }
    
    return points3d;
}


PointProjector::~PointProjector() {
}

