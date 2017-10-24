#ifndef KINECTGRABBER_H
#define KINECTGRABBER_H

#include <QObject>
#include <QElapsedTimer>

#include <Kinect.h>

#define COLOR_WIDTH  1920
#define COLOR_HEIGHT 1080
#define DEPTH_WIDTH   512
#define DEPTH_HEIGHT  424

class QLabel;

class KinectGrabber : public QObject
{    
    Q_OBJECT

public:
    KinectGrabber();
    ~KinectGrabber();

    void ConnectToKinect();
    void StartStream();
    void StartFrameGrabbingLoop();

signals:
    void ColorFrameAvailable(uchar* colorData);
    void DepthFrameAvailable(uchar* depthData);
    void FPSStatusMessage(float fps);
    void PointCloudDataAvailable(CameraSpacePoint* p, RGBQUAD* c, size_t size);
private:

    void ProcessMultiFrame();
    bool ProcessColor();
    bool ProcessDepth();
    bool CreatePointCloud();

    /**
     * @brief hr Current Status for Kinect API Calls
     */
    HRESULT hr;

    IKinectSensor* sensor;
    IMultiSourceFrameReader* reader;
    IMultiSourceFrame* multiFrame;
    ICoordinateMapper* coordinateMapper;

    // Color
    IColorFrameReference* colorFrameReference;
    IColorFrame* colorFrame;
    RGBQUAD*     colorBuffer;
    UINT32       colorBufferSize;

    // Depth
    IDepthFrameReference* depthFrameReference;
    IDepthFrame* depthFrame;
    UINT16*      depthBuffer;
    UINT32       depthBufferSize;
    UINT8*       depthBuffer8Bit;
    UINT32       depthBuffer8BitSize;

    // Pointcloud
    std::vector<CameraSpacePoint> pointCloudPoints;
    std::vector<RGBQUAD>          pointCloudColors;

    // Threading variables
    WAITABLE_HANDLE frameHandle;
    DWORD  frameGrabberThreadID;
    HANDLE frameGrabberThreadHandle;

    QElapsedTimer* timer;

};

#endif // KINECTGRABBER_H
