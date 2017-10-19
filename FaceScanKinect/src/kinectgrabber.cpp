#include "KinectGrabber.h"
#include <QtDebug>

template<class Resource>
void SafeRelease(Resource*& resource) {
    if (resource != NULL) {
        resource->Release();
        resource = NULL;
    }
}

INT32 FrameGrabberThread(void* params) {
    KinectGrabber* grabber = (KinectGrabber*) params;
    grabber->GrabFrame();
    return 0;
}

KinectGrabber::KinectGrabber() {
}

void KinectGrabber::GrabFrame() {
    HANDLE handles[] = { (HANDLE)frameHandle };

    bool quit = false;
    DWORD waitStatus;
    while(!quit) {
        waitStatus = WaitForMultipleObjects(1, handles, FALSE, 1000);
        switch(waitStatus) {
        case WAIT_TIMEOUT:
            qInfo("Timeout waiting for Kinect Event");
            break;

        case WAIT_OBJECT_0:
            IMultiSourceFrameArrivedEventArgs* frameEventArgs = nullptr;
            reader->GetMultiSourceFrameArrivedEventData(frameHandle, &frameEventArgs);
            ProcessFrame();
            frameEventArgs->Release();
            break;
        }
    }
}

void KinectGrabber::ProcessFrame() {
   hr = reader->AcquireLatestFrame(&frame);

   if (FAILED(hr)) {
       qCritical("Could not acquire frame");
       return;
   }

   // TODO: Actually get Data here.

//    frame->get_ColorFrameReference()

   SafeRelease(frame);
}

void KinectGrabber::ConnectToKinect() {
    // Get Sensor
    hr = GetDefaultKinectSensor(&sensor);
    if (FAILED(hr)) {
        qCritical("Default KinectSensor could not be retrieved.");
        return;
    }

    // Open Sensor
    hr = sensor->Open();
    if (FAILED(hr)) {
        qCritical("KinectSensor could not be opened.");
        return;
    }

    // Get Frame Reader
    hr = sensor->OpenMultiSourceFrameReader(FrameSourceTypes_Depth |
                                            FrameSourceTypes_Color,
                                            &reader);
    if (FAILED(hr)) {
        qCritical("MultiSourceFrameReader could not be opened.");
        return;
    }

    // Get Coordinate Mapper
    hr = sensor->get_CoordinateMapper(&coordinateMapper);
    if (FAILED(hr)) {
        qCritical("Coordinate Mapper could not be retrieved.");
        return;
    }
}

void KinectGrabber::StartStream() {
    hr = reader->SubscribeMultiSourceFrameArrived(&frameHandle);
    if (FAILED(hr)) {
        qCritical("Failed to create Stream Handle. Cannot Start Stream");
        return;
    }

    frameGrabberThreadHandle =
            CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)&FrameGrabberThread, this, 0, &frameGrabberThreadID);

    if (frameGrabberThreadHandle == NULL) {
        qCritical("Frame Grabber Thread could not be created");
    }
}
