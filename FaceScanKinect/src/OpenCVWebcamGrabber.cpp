#include "OpenCVWebcamGrabber.h"

#include <opencv2/opencv.hpp>

#include <QDebug>

#include "KinectGrabber.h"

OpenCVWebcamGrabber::OpenCVWebcamGrabber(MemoryPool* memory,
                                         LandmarkDetector::CLNF* faceTrackingModel,
                                         LandmarkDetector::FaceModelParameters* faceTrackingParameters)
    : memory_(memory),
      faceTrackingModel_(faceTrackingModel),
      faceTrackingParameters_(faceTrackingParameters)
{
    doFaceTracking = true;
    StartStream();
}

static INT32 OpenCVFrameGrabberThread(void* params) {
    OpenCVWebcamGrabber* grabber = (OpenCVWebcamGrabber*) params;
    grabber->StartFrameGrabbingLoop();
    return 0;
}

void OpenCVWebcamGrabber::StartFrameGrabbingLoop() {
    cv::VideoCapture cap;

    if (!cap.open(0)) {
        qCritical() << "Cannot open webcam";
        return;
    }

    while (true) {
        cv::Mat frame;
        cap >> frame;

        cv::cvtColor(frame, frame, CV_BGR2RGBA);
        cv::resize(frame, frame, cv::Size(COLOR_WIDTH, COLOR_HEIGHT));

        memcpy(memory_->gatherBuffer.colorBuffer, frame.data, COLOR_BUFFER_SIZE);

        QThread* faceTrackingThread = nullptr;
        if (doFaceTracking) {
            faceTrackingThread = new FaceTrackingThread(memory_->gatherBuffer.colorBuffer,
                                                        faceTrackingModel_,
                                                        faceTrackingParameters_);
            faceTrackingThread->start();
        }

        if (doFaceTracking) {
            faceTrackingThread->wait();
            faceTrackingThread->deleteLater();
        }

        emit FrameReady();
    }

    cap.release();
}

void OpenCVWebcamGrabber::StartStream()
{
    // Start thread and pass this object as thread parameter
    frameGrabberThreadHandle =
            CreateThread(NULL,                                              // Security Params
                         0,                                                 // Stack Size
                         (LPTHREAD_START_ROUTINE)&OpenCVFrameGrabberThread, // Start function
                         this,                                              // Start function params
                         0,                                                 // Creation flags
                         &frameGrabberThreadID);                            // Thread ID

}
