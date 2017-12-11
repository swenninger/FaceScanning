/* 
 * File:   PointProjector.h
 * Author: rzorn
 *
 * Created on 26. Juli 2015, 14:49
 */

#ifndef POINTPROJECTOR_H
#define	POINTPROJECTOR_H

#define _USE_MATH_DEFINES
#include <cmath>
#include <vector>
#include <opencv2/opencv.hpp>

/**
 * Code taken mostly from OpenNI2 source code.
 */
class PointProjector {
public:

    PointProjector(double hAngle, double vAngle, cv::Size size, float toMeters) :
    size(size), xzfactor(tan(hAngle / 360 * M_PI)*2),
    yzfactor(tan(vAngle / 360 * M_PI)*2), toMeters(toMeters) {
        coeffX = size.width / xzfactor;
        coeffY = size.height / yzfactor;
    }

    PointProjector(const PointProjector& other) :
    size(other.size), toMeters(other.toMeters), xTan(other.xTan), yTan(other.yTan), xzfactor(other.xzfactor), yzfactor(other.yzfactor), coeffX(other.coeffX), coeffY(other.coeffY) {
    }

    const cv::Size& getSize() const {
        return size;
    }
    
    std::vector<cv::Point3f> get3DPoints(const std::vector<cv::Point2i>& points2d, const cv::Mat& depthimage) const;

    cv::Point3f project2Dto3D(const cv::Point2f point, const uint16_t value) const;

    cv::Point2f project3Dto2D(const cv::Point3f point3f, uint16_t* value = NULL) const;

    cv::Point2f inline project3Dto2D(const std::vector<float>& p) const {
        if (p.size() == 3) {
            cv::Point3f p3d(p[0], p[1], p[2]);
            return project3Dto2D(p3d);
        } else {
            std::cerr << "Warning: Input vector had " << p.size() << " elements!" << std::endl;
            return cv::Point2f();
        }
    }

    static const PointProjector ASUS_XTION_VGA_100_UM;
    static const PointProjector ASUS_XTION_QVGA_100_UM;
    static const PointProjector KINECT2;

    virtual ~PointProjector();
private:
    const cv::Size size;
    std::vector<double> xTan;
    std::vector<double> yTan;
    const double toMeters;
    double xzfactor;
    double yzfactor;
    double coeffX;
    double coeffY;
};

#endif	/* POINTPROJECTOR_H */

