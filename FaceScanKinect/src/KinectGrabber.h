#ifndef KINECTGRABBER_H
#define KINECTGRABBER_H

#include <QObject>
#include <QElapsedTimer>

#include <Kinect.h>

#include "util.h"

#define COLOR_WIDTH  1920
#define COLOR_HEIGHT 1080
#define DEPTH_WIDTH   512
#define DEPTH_HEIGHT  424

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
    RGBQUAD*     colorBuffer;
    UINT32       colorBufferSize;

    // Depth
    IDepthFrameReference* depthFrameReference;
    IDepthFrame* depthFrame;
    UINT16*      depthBuffer;
    UINT32       depthBufferSize;
    UINT8*       depthBuffer8Bit;
    UINT32       depthBuffer8BitSize;

    // BodyIndex
    IBodyIndexFrameReference* bodyIndexFrameReference;
    IBodyIndexFrame*  bodyIndexFrame;
    UINT8*            bodyIndexBuffer;
    UINT32            bodyIndexBufferSize;

    // Pointcloud
    std::vector<Vec3f> pointCloudPoints;
    std::vector<RGB3f> pointCloudColors;
    bool captureNonTrackedBodies;

    // Threading variables
    WAITABLE_HANDLE frameHandle;
    DWORD  frameGrabberThreadID;
    HANDLE frameGrabberThreadHandle;

    QElapsedTimer* timer;

};

#endif // KINECTGRABBER_H
