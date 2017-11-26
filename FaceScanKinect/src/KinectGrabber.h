#ifndef KINECTGRABBER_H
#define KINECTGRABBER_H

#include <QObject>
#include <QElapsedTimer>

#include <Kinect.h>

#include "util.h"

class QLabel;

/**
 * @brief The CapturedFrame struct stores all data belonging to a Frame.
 *
 * TODO: buffersizes for color and depth are obsolete/contant
 *
 */
struct CapturedFrame {
    uchar* depthBuffer;
    size_t   depthBufferSize;

    uchar* colorBuffer;
    size_t   colorBufferSize;

    PointCloud pointCloud;
};


/**
 * @brief The KinectGrabber class is responsible for grabbing the Data from the Kinect.
 *
 * It Starts a Capture Thread which retrieves MultiFrames via the Microsoft Kinect API.
 *
 * These are stored in internal buffers, and once a frame is complete, a signal
 * containing the buffers is emitted.
 *
 *
 *
 * TODO:
 *      std::vector is used for the point clouds. Think about defining a max number of points and switch to array,
 *      keeping track of the current number of points
 *
 */
class KinectGrabber : public QObject
{    
    Q_OBJECT

public:
    KinectGrabber();
    ~KinectGrabber();

    void ConnectToKinect();
    void StartStream();
    void StartFrameGrabbingLoop();

public slots:
    void RetrieveTrackedBodiesOnlySettingsChanged(int captureTrackedBodiesOnlyState);

signals:
    void FrameReady(CapturedFrame CapturedFrame);
    void FrameReady();
    void FPSStatusMessage(float fps);

private:
    void ProcessMultiFrame();
    bool ProcessColor();
    bool ProcessDepth();
    bool ProcessBodyIndex();
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

    // Depth
    IDepthFrameReference* depthFrameReference;
    IDepthFrame* depthFrame;
    UINT16*      depthBuffer;
    UINT32       depthBufferSize;

    // BodyIndex
    IBodyIndexFrameReference* bodyIndexFrameReference;
    IBodyIndexFrame*  bodyIndexFrame;
    UINT8*            bodyIndexBuffer;
    UINT32            bodyIndexBufferSize;

    // Pointcloud
    bool captureNonTrackedBodies;

    // Threading variables
    WAITABLE_HANDLE frameHandle;
    DWORD  frameGrabberThreadID;
    HANDLE frameGrabberThreadHandle;

    QElapsedTimer* timer;

};

#endif // KINECTGRABBER_H
