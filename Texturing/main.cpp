#define _USE_MATH_DEFINES
#include <cmath>

#include <Kinect.h>


#include <iostream>
#include <iomanip>
#include <fstream>
//#include <opencv2/opencv.hpp>
#include <opencv2/core.hpp>
#include <opencv2/highgui.hpp>
std::string data_directory = "..\\..\\data\\";

#include "PointProjector.h"

struct face {
    int v1, v2, v3;
    int vn1, vn2, vn3;
    int vt1, vt2, vt3;
};

std::vector<cv::Point3f> vertices;
std::vector<cv::Vec3f> normals;
std::vector<cv::Vec2f> textureCoordinates;
std::vector<face> faces;

static void load_obj() {
    std::ifstream file(data_directory + "mesh\\mesh.obj");

    size_t numVertices = 0;
    size_t numFaces = 0;
    size_t numNormals = 0;
    size_t numTexcoords = 0;

    float x, y, z;
    face f;

    std::string line;

    while(std::getline(file, line)) {

        size_t size = line.size();


        if (size > 3) {
            if        (line[0] == 'v' && line[1] == 'n') {
                sscanf(line.c_str(), "%*s %f %f %f", &x, &y, &z);
                normals.push_back(cv::Vec3f(x, y, z));
                numNormals++;
            } else if (line[0] == 'v' && line[1] == 't') {
                sscanf(line.c_str(), "%*s %f %f %f", &x, &y, &z);
                textureCoordinates.push_back(cv::Vec2f(x, y));
                numTexcoords++;
            } else if (line[0] == 'v' && line[1] == ' ') {
                sscanf(line.c_str(), "%*s %f %f %f", &x, &y, &z);
                vertices.push_back(cv::Point3f(x, y, z));
                numVertices++;
            } else if (line[0] == 'f' && line[1] == ' ') {
                sscanf(line.c_str(), "%*s %d/%d/%d %d/%d/%d %d/%d/%d",
                       &f.v1, &f.vt1, &f.vn1,
                       &f.v2, &f.vt2, &f.vn2,
                       &f.v3, &f.vt3, &f.vn3
                       );
                faces.push_back(f);
                numFaces++;
            }
        }
    }

#if 0
    std::cout << normals[3][0] << " " << normals[3][1] << " " << normals[3][2] << std::endl;
    std::cout << vertices[3].x << " " << vertices[3].y << " " << vertices[3].z << std::endl;
    std::cout << faces[3].v1 << "/" << faces[3].vt1 << "/" << faces[3].vn1 << " "
              << faces[3].v2 << "/" << faces[3].vt2 << "/" << faces[3].vn2 << " "
              << faces[3].v3 << "/" << faces[3].vt3 << "/" << faces[3].vn3 <<  std::endl;
    std::cout << numVertices << " " << numFaces << " " << numNormals << " " << numTexcoords << std::endl;
#endif

    file.close();
}

static inline cv::Vec2f getTexCoord(int indexStartingAtOne) {
    return textureCoordinates[indexStartingAtOne - 1];
}

static inline cv::Vec3f getNormal(int indexStartingAtOne) {
    return normals[indexStartingAtOne - 1];
}

static inline cv::Point3f getVertex(int indexStartingAtOne) {
    return vertices[indexStartingAtOne - 1];
}

cv::Vec3b getColorSubpix(const cv::Mat& img, cv::Point2f pt)
{
    assert(!img.empty());
    assert(img.channels() == 3);

    int x = (int)pt.x;
    int y = (int)pt.y;

    int x0 = cv::borderInterpolate(x,   img.cols, cv::BORDER_REFLECT_101);
    int x1 = cv::borderInterpolate(x+1, img.cols, cv::BORDER_REFLECT_101);
    int y0 = cv::borderInterpolate(y,   img.rows, cv::BORDER_REFLECT_101);
    int y1 = cv::borderInterpolate(y+1, img.rows, cv::BORDER_REFLECT_101);

    float a = pt.x - (float)x;
    float c = pt.y - (float)y;

    uchar b = (uchar)cvRound((img.at<cv::Vec3b>(y0, x0)[0] * (1.f - a) + img.at<cv::Vec3b>(y0, x1)[0] * a) * (1.f - c)
                           + (img.at<cv::Vec3b>(y1, x0)[0] * (1.f - a) + img.at<cv::Vec3b>(y1, x1)[0] * a) * c);
    uchar g = (uchar)cvRound((img.at<cv::Vec3b>(y0, x0)[1] * (1.f - a) + img.at<cv::Vec3b>(y0, x1)[1] * a) * (1.f - c)
                           + (img.at<cv::Vec3b>(y1, x0)[1] * (1.f - a) + img.at<cv::Vec3b>(y1, x1)[1] * a) * c);
    uchar r = (uchar)cvRound((img.at<cv::Vec3b>(y0, x0)[2] * (1.f - a) + img.at<cv::Vec3b>(y0, x1)[2] * a) * (1.f - c)
                           + (img.at<cv::Vec3b>(y1, x0)[2] * (1.f - a) + img.at<cv::Vec3b>(y1, x1)[2] * a) * c);

    return cv::Vec3b(b, g, r);
}

void loadMappingFile(ColorSpacePoint* cameraToColorMapping) {
    std::ifstream mappingFile("mapping.txt");

    float x,y;
    int count = 0;
    while(mappingFile >> x >> y) {
        cameraToColorMapping[count++] = {x,y};
    }

    mappingFile.close();
} 

int main()
{

    HRESULT hr;
    // Get Sensor
    IKinectSensor* sensor;
    ICoordinateMapper* coordinateMapper;

    hr = GetDefaultKinectSensor(&sensor);
    if (FAILED(hr)) { return -1;  }

	sensor->Open();

    // Get Coordinate Mapper
    hr = sensor->get_CoordinateMapper(&coordinateMapper);
    if (FAILED(hr)) { return -2; }

    load_obj();

    std::string color_file = data_directory + "PointcloudSample\\snapshot_color.bmp";
    std::cout << color_file << std::endl;

    const int SIZE = 2048 * 2;
    cv::Mat texture = cv::Mat(SIZE,SIZE, CV_8UC3);
    cv::Mat color_kinect = cv::imread(color_file, CV_LOAD_IMAGE_COLOR);
    cv::Mat samplePoints = color_kinect.clone();

    float minx, miny, maxx, maxy;

    int numBadMappings = 0;
    int numBadMappingsFromInterp = 0;

    ColorSpacePoint* cameraToColorMapping = new ColorSpacePoint[vertices.size()];
#if 1
    loadMappingFile(cameraToColorMapping); 

    for (int i = 0; i < vertices.size(); ++i) {
        samplePoints.at<cv::Vec3b>(cameraToColorMapping[i].Y, cameraToColorMapping[i].X) = cv::Vec3b(200, 200, 50);
    }
#endif

    for (auto& face : faces) {
#if 0
        cv::Point3f v1 = getVertex(face.v1);
        cv::Point3f v2 = getVertex(face.v2);
        cv::Point3f v3 = getVertex(face.v3);

        cv::Vec3f v1v2 = v2-v1;
        cv::Vec3f v1v3 = v3-v1;

        CameraSpacePoint cp1 = {v1.x, v1.y, v1.z};
        ColorSpacePoint csp1;
        hr = coordinateMapper->MapCameraPointToColorSpace(cp1, &csp1);
        if (SUCCEEDED(hr) && !std::isinf(csp1.X) && !std::isinf(csp1.Y) &&
            csp1.X > 0 && csp1.Y > 0) {
            cameraToColorMapping[face.v1 - 1] = csp1;
        }
        else {
            cameraToColorMapping[face.v1 - 1] = {0.0f, 0.0f};
        }


        CameraSpacePoint cp2 = {v2.x, v2.y, v2.z};
        ColorSpacePoint csp2;
        hr = coordinateMapper->MapCameraPointToColorSpace(cp2, &csp2);
        if (SUCCEEDED(hr) && !std::isinf(csp2.X) && !std::isinf(csp2.Y) &&
            csp2.X > 0 && csp2.Y > 0) {
            cameraToColorMapping[face.v2 - 1] = csp2;
        }
        else {
            cameraToColorMapping[face.v2 - 1] = {0.0f, 0.0f};
        }


        CameraSpacePoint cp3 = {v3.x, v3.y, v3.z};
        ColorSpacePoint csp3;
        hr = coordinateMapper->MapCameraPointToColorSpace(cp3, &csp3);
        if (SUCCEEDED(hr) && !std::isinf(csp3.X) && !std::isinf(csp3.Y) &&
            csp3.X > 0 && csp3.Y > 0) {
            cameraToColorMapping[face.v3 - 1] = csp3;
        }
        else {
            cameraToColorMapping[face.v3 - 1] = {0.0f, 0.0f};
        }

        cv::Point2f p1 = cv::Point2f(csp1.X, csp1.Y); // PointProjector::KINECT2.project3Dto2D(v1);
        cv::Point2f p2 = cv::Point2f(csp2.X, csp2.Y); //PointProjector::KINECT2.project3Dto2D(v2);
        cv::Point2f p3 = cv::Point2f(csp3.X, csp3.Y); //PointProjector::KINECT2.project3Dto2D(v3);

        cv::Vec2f p1p2 = p2 - p1;
        
        cv::Vec2f p1p3 = p3 - p1;
        
        cv::Vec2f uv1 = SIZE * getTexCoord(face.vt1);
        cv::Vec2f uv2 = SIZE * getTexCoord(face.vt2);
        cv::Vec2f uv3 = SIZE * getTexCoord(face.vt3);

        cv::Point2f uvi1 = cv::Point2f(uv1[0], SIZE - uv1[1]);
        cv::Point2f uvi2 = cv::Point2f(uv2[0], SIZE - uv2[1]);
        cv::Point2f uvi3 = cv::Point(uv3[0], SIZE - uv3[1]);

        //texture.at<cv::Vec3b>(uvi1) = cv::Vec3b(200, 200, 50); // color_kinect.at<cv::Vec3b>(p1.x, color_kinect.rows - p1.y);
        //texture.at<cv::Vec3b>(uvi2) = cv::Vec3b(200, 200, 50); // color_kinect.at<cv::Vec3b>(p2.x, color_kinect.rows - p2.y);
        //texture.at<cv::Vec3b>(uvi3) = cv::Vec3b(200, 200, 50); // color_kinect.at<cv::Vec3b>(p3.x, color_kinect.rows - p3.y);

        //cv::line(texture, uvi1, uvi2, cv::Scalar(200, 200, 50));
        //cv::line(texture, uvi2, uvi3, cv::Scalar(200, 200, 50));
        //cv::line(texture, uvi3, uvi1, cv::Scalar(200, 200, 50));

        cv::Vec2f v_0 = uvi2 - uvi3;
        cv::Vec2f v_1 = uvi1 - uvi3;


        // TODO: Different triangle rasterization!

        int maxx, maxy, minx, miny;
        maxx = std::ceil(std::max(uvi3.x, std::max(uvi1.x, uvi2.x)));
        maxy = std::ceil(std::max(uvi3.y, std::max(uvi1.y, uvi2.y)));
        minx = std::floor(std::min(uvi3.x, std::min(uvi1.x, uvi2.x)));
        miny = std::floor(std::min(uvi3.y, std::min(uvi1.y, uvi2.y)));

        int rowOffset = 0;
        int colOffset = 0;


        for (int row = miny; row < maxy; ++row, ++rowOffset) {
            for (int col = minx, colOffset = 0; col < maxx; ++col, ++colOffset) {
 
                cv::Point2f p = cv::Point2f(col, row); // - vt1.x, row - vt1.y);
                cv::Vec2f v_2 = p - uvi3;

                float dot_00 = v_0.dot(v_0);
                float dot_01 = v_0.dot(v_1);
                float dot_02 = v_0.dot(v_2);
                float dot_11 = v_1.dot(v_1);
                float dot_12 = v_1.dot(v_2);
                
				float invDenom = 1 / (dot_00 * dot_11 - dot_01 * dot_01);

                float u = (dot_11 * dot_02 - dot_01 * dot_12) * invDenom;
                float v = (dot_00 * dot_12 - dot_01 * dot_02) * invDenom;

                bool inTriangle = u >= 0.0f && v >= 0.0f && (u + v <= 1.0f);

                if (inTriangle) {
                    cv::Point2f samplePoint = cv::Point2f(p1.x, p1.y);
#if 1
                    cv::Vec3f samplePoint3D = cv::Vec3f(v1.x, v1.y, v1.z) + 
                                                          u * v1v2 + 
                                                          v * v1v3;

                    // TODO: Is this perspectively correct? 

                    cv::Vec3f v1Vec = v1;
                    cv::Vec3f v2Vec = v2;
                    cv::Vec3f v3Vec = v3;
                    samplePoint3D = (1.0f - u - v) * v1Vec / v1.z +
                                                u  * v2Vec / v2.z + 
                                                v  * v3Vec / v3.z;

                    float z = 1 / ((1.0f - u - v) * v1.z + 
                                                u * v2.z + 
                                                v * v3.z);
                    samplePoint3D /= z;

                    CameraSpacePoint samplePoint3DCSP = {samplePoint3D[0], samplePoint3D[1], samplePoint3D[2]};
                    ColorSpacePoint  samplePoint2DCSP;
                    hr = coordinateMapper->MapCameraPointToColorSpace(samplePoint3DCSP, &samplePoint2DCSP);
                    if (SUCCEEDED(hr) && !std::isinf(samplePoint2DCSP.X) && !std::isinf(samplePoint2DCSP.Y) &&
                        samplePoint2DCSP.X > 0 && samplePoint2DCSP.Y > 0 &&
                        samplePoint2DCSP.X < 1920 && samplePoint2DCSP.Y < 1079) {
                    }
                    else {
                        numBadMappings++;
                        numBadMappingsFromInterp++;
                        continue;
                    }
                    samplePoint.x = samplePoint2DCSP.X;
                    samplePoint.y = samplePoint2DCSP.Y;


#else 
                       

                    // Perspective incorrect interpolation
      //              samplePoint = cv::Vec2f(p1.x, p1.y) + u * p1p2 + v * p1p3;

                    // Perspective correct
                    // v1 maps to p1
                    // v2 maps to p2
                    // v3 maps to p3
                    samplePoint = cv::Vec2f(p1.x, p1.y) + u * p1p2 + v * p1p3;
                    //samplePoint.x /= (v1.z + u * v1v2[2] + v * v1v3[2]);
                    //samplePoint.y /= (v1.z + u * v1v2[2] + v * v1v3[2]);

#endif
                    //texture.at<cv::Vec3b>(p) = color_kinect.at<cv::Vec3b>(samplePoint);
                    texture.at<cv::Vec3b>(p) = getColorSubpix(color_kinect, samplePoint);
                    samplePoints.at<cv::Vec3b>(samplePoint) = (0.95 * samplePoints.at<cv::Vec3b>(samplePoint)) + (0.05 * cv::Vec3b(200, 200, 50));
                }

#if 0
                float alpha = ((uvi2.y - uvi3.y)*(p.x - uvi3.x) + (uvi3.x - uvi2.x)*(p.y - uvi3.y)) /
                              ((uvi2.y - uvi3.y)*(uvi1.x - uvi3.x) + (uvi3.x - uvi2.x)*(uvi1.y - uvi3.y));
                float beta = ((uvi3.y - uvi1.y)*(p.x - uvi3.x) + (uvi1.x - uvi3.x)*(p.y - uvi3.y)) /
                             ((uvi2.y - uvi3.y)*(uvi1.x - uvi3.x) + (uvi3.x - uvi2.x)*(uvi1.y - uvi3.y));
                float gamma = 1.0f - alpha - beta;

                // float s = (float)crossProduct(q, vs2) / crossProduct(vs1, vs2);
                // float t = (float)crossProduct(vs1, q) / crossProduct(vs1, vs2);

                // if ( (s >= 0) && (t >= 0) && (s + t <= 1))
                
                if (alpha > 0 && beta > 0 && gamma > 0 && 
                    alpha < 1.0f && beta < 1.0f && gamma < 1.0f)
                { /* inside triangle */
             //     if (!std::isnan(p1.x) && !std::isnan(p1.y))
                }

#endif
            }
        }        
#if 0
        // std::cout << p1.x << ", " << p1.y << std::endl;
        // std::cout << p2.x << ", " << p2.y << std::endl;
        // std::cout << p3.x << ", " << p3.y << std::endl;

        // if (!std::isnan(p1.x) && !std::isnan(p1.y)) texture.at<cv::Vec3b>(uvi1) = color_kinect.at<cv::Vec3b>(p1.x, color_kinect.rows - p1.y);
        // if (!std::isnan(p2.x) && !std::isnan(p2.y)) texture.at<cv::Vec3b>(uvi2) = color_kinect.at<cv::Vec3b>(p2.x, color_kinect.rows - p2.y);
        // if (!std::isnan(p3.x) && !std::isnan(p3.y)) texture.at<cv::Vec3b>(uvi3) = color_kinect.at<cv::Vec3b>(p3.x, color_kinect.rows - p3.y);
#endif
#endif
    }

#if 0
    std::ofstream mappingFile("mapping.txt");

    for (int i = 0; i < vertices.size(); ++i) {
        mappingFile << cameraToColorMapping[i].X << " " << cameraToColorMapping[i].Y << std::endl;
    }

    mappingFile.close();
    #endif

    std::cout << "Bad mappings: " << numBadMappings << std::endl;
    std::cout << "Bad mappings from interp: " << numBadMappingsFromInterp << std::endl;

    cv::imwrite("sampled_color.jpg", samplePoints);
    cv::imwrite("test.jpg", texture);

    
    cv::imshow("SampledPoints", samplePoints);
    cv::imshow("Result", texture);

    cv::waitKey(0);

    return 0;
}
