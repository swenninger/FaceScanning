#ifndef KINECTGRABBER_H
#define KINECTGRABBER_H

#include "Kinect.h"

#define COLOR_WIDTH  1920
#define COLOR_HEIGHT 1080
#define DEPTH_WIDTH   512
#define DEPTH_HEIGHT  424

class QLabel;

class KinectGrabber
{    
public:
    KinectGrabber(QLabel* parent);

    void ConnectToKinect();
    void StartStream();
    void StartFrameGrabbingLoop();

private:

    void ProcessMultiFrame();
    void ProcessColor();
    void ProcessDepth();

    QLabel* parent;

    /**
     * @brief hr Current Status for Kinect API Calls
     */
    HRESULT hr;

    IKinectSensor* sensor;
    IMultiSourceFrameReader* reader;
    IMultiSourceFrame* frame;
    ICoordinateMapper* coordinateMapper;

    IColorFrame* colorFrame;
    IColorFrameReference* colorFrameReference;
    RGBQUAD* colorBuffer;
    UINT32   colorBufferSize;

    IDepthFrame* depthFrame;
    IDepthFrameReference* depthFrameReference;
    UINT16* depthBuffer;
    UINT32  depthBufferSize;


    WAITABLE_HANDLE frameHandle;

    DWORD  frameGrabberThreadID;
    HANDLE frameGrabberThreadHandle;
};

#endif // KINECTGRABBER_H
