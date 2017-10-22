#include "KinectGrabber.h"
#include <QtDebug>
#include <QImage>
#include <QPixmap>
#include <QLabel>


template<class Resource>
void SafeRelease(Resource*& resource) {
    if (resource != NULL) {
        resource->Release();
        resource = NULL;
    }
}

INT32 FrameGrabberThread(void* params) {
    KinectGrabber* grabber = (KinectGrabber*) params;
    grabber->StartFrameGrabbingLoop();
    return 0;
}

KinectGrabber::KinectGrabber(QLabel *parent) {
    this->parent = parent;

    colorBufferSize = sizeof(RGBQUAD) * COLOR_HEIGHT * COLOR_WIDTH;
    colorBuffer = new RGBQUAD[COLOR_HEIGHT * COLOR_WIDTH];

    // TODO: Check again when using depth
    depthBufferSize = DEPTH_HEIGHT * DEPTH_WIDTH;
    depthBuffer = new UINT16[DEPTH_HEIGHT * DEPTH_WIDTH];
}

void KinectGrabber::StartFrameGrabbingLoop() {
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
            ProcessMultiFrame();
            frameEventArgs->Release();
            break;
        }
    }
}

void KinectGrabber::ProcessMultiFrame() {
    hr = reader->AcquireLatestFrame(&frame);

    if (FAILED(hr)) { qCritical("Could not acquire frame"); return; }

    qInfo("Frame Received");

    hr = frame->get_ColorFrameReference(&colorFrameReference);
    if (FAILED(hr)) { qCritical("No Color Frame"); return; }
    ProcessColor();

    hr = frame->get_DepthFrameReference(&depthFrameReference);
    if (FAILED(hr)) { qCritical("No Depth Frame"); }
    ProcessDepth();

    SafeRelease(frame);
}

void KinectGrabber::ProcessColor() {
    hr = colorFrameReference->AcquireFrame(&colorFrame);

    if (SUCCEEDED(hr)) {
        hr = colorFrame->CopyConvertedFrameDataToArray(colorBufferSize,
                                                       (BYTE*)colorBuffer,
                                                       ColorImageFormat_Rgba);

        if (SUCCEEDED(hr)) {
            qInfo("Raw Color Data");

            QPixmap pixmap = QPixmap::fromImage(QImage((uchar*) colorBuffer,
                                                       COLOR_WIDTH,
                                                       COLOR_HEIGHT,
                                                       QImage::Format_RGBA8888));

            parent->setPixmap(pixmap);

        } else {
            // TODO: Logging
        }

        SafeRelease(colorFrame);
    } else {
        // TODO: Logging
    }

    SafeRelease(colorFrameReference);
}

void KinectGrabber::ProcessDepth() {
    SafeRelease(depthFrameReference);
}


void KinectGrabber::ConnectToKinect() {
    // Get Sensor
    hr = GetDefaultKinectSensor(&sensor);
    if (FAILED(hr)) { qCritical("Default KinectSensor could not be retrieved."); return; }

    // Open Sensor
    hr = sensor->Open();
    if (FAILED(hr)) { qCritical("KinectSensor could not be opened."); return; }

    // Get Frame Reader
    hr = sensor->OpenMultiSourceFrameReader(FrameSourceTypes_Depth |
                                            FrameSourceTypes_Color,
                                            &reader);
    if (FAILED(hr)) { qCritical("MultiSourceFrameReader could not be opened."); return; }

    // Get Coordinate Mapper
    hr = sensor->get_CoordinateMapper(&coordinateMapper);
    if (FAILED(hr)) { qCritical("Coordinate Mapper could not be retrieved."); return; }
}

void KinectGrabber::StartStream() {
    hr = reader->SubscribeMultiSourceFrameArrived(&frameHandle);
    if (FAILED(hr)) { qCritical("Failed to create Stream Handle. Cannot Start Stream"); return; }

    frameGrabberThreadHandle =
            CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)&FrameGrabberThread, this, 0, &frameGrabberThreadID);

    if (frameGrabberThreadHandle == NULL) {
        qCritical("Frame Grabber Thread could not be created");
    }
}
