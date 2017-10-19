#ifndef KINECTGRABBER_H
#define KINECTGRABBER_H

#include "Kinect.h"

class KinectGrabber
{    
public:
    KinectGrabber();

    void ConnectToKinect();
    void StartStream();
    void GrabFrame();

private:

    void ProcessFrame();

    /**
     * @brief hr Current Status for Kinect API Calls
     */
    HRESULT hr;

    IKinectSensor* sensor;
    IMultiSourceFrameReader* reader;
    IMultiSourceFrame* frame;
    ICoordinateMapper* coordinateMapper;

    WAITABLE_HANDLE frameHandle;

    DWORD  frameGrabberThreadID;
    HANDLE frameGrabberThreadHandle;
};

#endif // KINECTGRABBER_H
