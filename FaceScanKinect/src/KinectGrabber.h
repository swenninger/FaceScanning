#ifndef KINECTGRABBER_H
#define KINECTGRABBER_H

#include <QObject>

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

    void ConnectToKinect();
    void StartStream();
    void StartFrameGrabbingLoop();

signals:
    void ColorFrameAvailable(uchar* colorData);
    void DepthFrameAvailable(uchar* depthData);

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
    CameraSpacePoint* pointPositions;
    ColorSpacePoint*  pointColors;

    // Threading variables
    WAITABLE_HANDLE frameHandle;
    DWORD  frameGrabberThreadID;
    HANDLE frameGrabberThreadHandle;
};

#endif // KINECTGRABBER_H
