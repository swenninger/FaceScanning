#ifndef KINECTGRABBER_H
#define KINECTGRABBER_H

#include <QObject>

#include <Kinect.h>

struct FrameBuffer;


namespace LandmarkDetector {
    struct FaceModelParameters;
    class CLNF;
}

/**
 * @brief The KinectGrabber class is responsible for grabbing the Data from the Kinect.
 *
 * It Starts a Capture Thread which retrieves MultiFrames via the Microsoft Kinect API.
 *
 * Incoming Data is copied into internal buffers and once a Frame is completly gathered,
 * the FrameReady() Signal is emitted.
 */
class KinectGrabber : public QObject
{    
    Q_OBJECT

public:

    KinectGrabber(FrameBuffer* multiFrameBuffer,
                  LandmarkDetector::CLNF* faceTrackingModel,
                  LandmarkDetector::FaceModelParameters* faceTrackingParameters);
    ~KinectGrabber();

    void ConnectToKinect();
    void StartStream();
    void StartFrameGrabbingLoop();

    inline void ToggleFaceTracking() { doFaceTrackingToggleRequested = true; }

    inline ICoordinateMapper*  GetCoordinateMapper() { return coordinateMapper; }

signals:
    void FrameReady();

private:
    void ProcessMultiFrame();
    bool ProcessColor();
    bool ProcessDepth();
    bool ProcessBodyIndex();
    bool CreatePointCloud();

    LandmarkDetector::CLNF* faceTrackingModel_;
    LandmarkDetector::FaceModelParameters* faceTrackingParameters_;
    /**
     * @brief hr Current Status for Kinect API Calls
     */
    HRESULT hr;

    // Internal storage, passed from outside
    FrameBuffer* multiFrameBuffer;

    // Kinect API elements
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

    // We need a further buffer for depth, since depth comes as UINT16
    // and is converted into the UINT8 for the internal framebuffer
    //UINT16*      depthBuffer;
    //UINT32       depthBufferSize;

    // BodyIndex
    IBodyIndexFrameReference* bodyIndexFrameReference;
    IBodyIndexFrame*  bodyIndexFrame;

    // Body Index Buffer is only stored class-internally
    UINT8*            bodyIndexBuffer;
    UINT32            bodyIndexBufferSize;

    // Body
    IBodyFrameReference* bodyFrameReference;
    IBodyFrame* bodyFrame;

    CameraSpacePoint* tmpPositions;
    ColorSpacePoint*  tmpColors;

    bool doFaceTracking;
    bool doFaceTrackingToggleRequested;

    // Threading variables
    WAITABLE_HANDLE frameHandle;
    DWORD  frameGrabberThreadID;
    HANDLE frameGrabberThreadHandle;
};

#include <QThread>
class FaceTrackingThread : public QThread {
    Q_OBJECT

public:
    FaceTrackingThread(uint32_t* colorBuffer,
                       LandmarkDetector::CLNF* faceTrackingModel,
                       LandmarkDetector::FaceModelParameters* faceTrackingParameters)
        : colors(colorBuffer),
          faceTrackingModel_(faceTrackingModel),
          faceTrackingParameters_(faceTrackingParameters)
    { }

    void run() override;

private:
    uint32_t* colors;
    LandmarkDetector::CLNF *faceTrackingModel_;
    LandmarkDetector::FaceModelParameters *faceTrackingParameters_;
};

#endif // KINECTGRABBER_H
