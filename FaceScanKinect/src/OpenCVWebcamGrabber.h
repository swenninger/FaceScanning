#ifndef OPENCV_WEBCAM_GRABBER_H
#define OPENCV_WEBCAM_GRABBER_H

#include <QObject>
#include "MemoryPool.h"

#include <LandmarkCoreIncludes.h>

#include "windows.h"

class OpenCVWebcamGrabber : public QObject
{
    Q_OBJECT
public:
    OpenCVWebcamGrabber(MemoryPool* memory,
                        LandmarkDetector::CLNF* faceTrackingModel,
                        LandmarkDetector::FaceModelParameters* faceTrackingParameters);
    ~OpenCVWebcamGrabber() { }

    void StartFrameGrabbingLoop();

    void ToggleFaceTracking() { doFaceTracking = !doFaceTracking; }

signals:
    void FrameReady();

private:
    void StartStream();

    MemoryPool* memory_;

    bool doFaceTracking;

    // Threading variables
    // WAITABLE_HANDLE frameHandle;
    DWORD  frameGrabberThreadID;
    HANDLE frameGrabberThreadHandle;

    LandmarkDetector::CLNF* faceTrackingModel_;
    LandmarkDetector::FaceModelParameters* faceTrackingParameters_;

};

#endif // OPENCV_WEBCAM_GRABBER_H
