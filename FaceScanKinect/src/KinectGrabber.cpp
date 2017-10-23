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

KinectGrabber::KinectGrabber() {

    colorBufferSize = sizeof(RGBQUAD) * COLOR_HEIGHT * COLOR_WIDTH;
    colorBuffer     = new RGBQUAD[COLOR_HEIGHT * COLOR_WIDTH];

    // TODO: Check again when using depth
    depthBufferSize = DEPTH_HEIGHT * DEPTH_WIDTH;
    depthBuffer     = new UINT16[DEPTH_HEIGHT * DEPTH_WIDTH];

    depthBuffer8BitSize = depthBufferSize;
    depthBuffer8Bit     = new UINT8[depthBuffer8BitSize];
}

void KinectGrabber::StartFrameGrabbingLoop() {
    HANDLE handles[] = { (HANDLE)frameHandle };

    int maxTimeoutTries = 20;
    int numTimeouts = 0;

    bool quit = false;
    DWORD waitStatus;
    while(!quit) {
        waitStatus = WaitForMultipleObjects(1, handles, FALSE, 100);
        switch(waitStatus) {
        case WAIT_TIMEOUT:
            qInfo("Timeout waiting for Kinect Event");
            
            ++numTimeouts;

            if (numTimeouts >= maxTimeoutTries) {
                quit = true;
            }

            break;

        case WAIT_OBJECT_0:
            numTimeouts = 0;
            
            IMultiSourceFrameArrivedEventArgs* frameEventArgs = nullptr;
            reader->GetMultiSourceFrameArrivedEventData(frameHandle, &frameEventArgs);
            ProcessMultiFrame();
            frameEventArgs->Release();
            break;
        }
    }
}

void KinectGrabber::ProcessMultiFrame() {
    // Acquire MultiFrame
    hr = reader->AcquireLatestFrame(&frame);
    if (FAILED(hr)) { qCritical("Could not acquire frame"); return; }

    // Process individual components
    ProcessColor();
    ProcessDepth();

    SafeRelease(frame);
}

bool KinectGrabber::ProcessColor() {
    bool succeeded = false;

    hr = frame->get_ColorFrameReference(&colorFrameReference);
    if (FAILED(hr)) { qCritical("No Color Frame"); return false; }

    hr = colorFrameReference->AcquireFrame(&colorFrame);
    if (SUCCEEDED(hr)) {
        hr = colorFrame->CopyConvertedFrameDataToArray(colorBufferSize,
                                                       (BYTE*)colorBuffer,
                                                       ColorImageFormat_Rgba);
        if (SUCCEEDED(hr)) {
            succeeded = true;
            emit ColorFrameAvailable((uchar*)colorBuffer);
        } else {
            // TODO: Logging
        }

        SafeRelease(colorFrame);
    } else {
        // TODO: Logging
    }

    SafeRelease(colorFrameReference);
    return succeeded;
}
/*
inline UINT8 roundFloatToUINT8(float val) {
    UINT8 result = (UINT8) (val + (val > 0.0f ? 0.5f : -0.5f));
    return result;
}

inline UINT16 Clamp(UINT16 val, UINT16 min, UINT16 max) {
    UINT16 result = val;
    if (val < min) {
        result = min;
    } else if (val > max) {
        result = max;
    }
    return result;
}
*/

inline UINT8 SafeTruncateTo8Bit(INT32 val) {
    UINT8 result = val;
    if (val > 255) {
        result = 255;
    } else if (val < 0) {
        result = 0;
    }
    return result;
}

inline UINT8 FloatToUINT8(float val) {
    // Round to integer
    INT32 iVal = (INT32)floorf(val);
    // 0 to 255
    return SafeTruncateTo8Bit(iVal);
}

bool KinectGrabber::ProcessDepth() {
    bool succeeded = false;

    hr = frame->get_DepthFrameReference(&depthFrameReference);
    if (FAILED(hr)) { qCritical("No Depth Frame"); return false; }

    hr = depthFrameReference->AcquireFrame(&depthFrame);
    if (SUCCEEDED(hr)) {
        UINT16 minDistance, maxDistance;
        bool minDistanceAvailabe, maxDistanceAvailable;

        minDistanceAvailabe  = SUCCEEDED(depthFrame->get_DepthMinReliableDistance(&minDistance));
        maxDistanceAvailable = SUCCEEDED(depthFrame->get_DepthMaxReliableDistance(&maxDistance));
        hr = depthFrame->AccessUnderlyingBuffer(&depthBufferSize, &depthBuffer);

        if (SUCCEEDED(hr) && minDistanceAvailabe && maxDistanceAvailable) {
            succeeded = true;
            float scale = 255.0f / (maxDistance - minDistance);
            int numPixels = DEPTH_HEIGHT * DEPTH_WIDTH;

            for (int pixel = 0; pixel < numPixels; ++pixel) {
                INT32 depth = (INT32)depthBuffer[pixel];
                depth -= (INT32)minDistance;
                float val = depth * scale;
                depthBuffer8Bit[pixel] = FloatToUINT8(val);
            }

            emit DepthFrameAvailable((uchar*) depthBuffer8Bit);
        } else {
            // TODO: Logging
        }

        SafeRelease(depthFrame);
    } else {
        // TODO: Logging
    }

    SafeRelease(depthFrameReference);
    return succeeded;
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

    // Start thread and pass this object as thread parameter
    frameGrabberThreadHandle =
            CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)&FrameGrabberThread, this, 0, &frameGrabberThreadID);

    if (frameGrabberThreadHandle == NULL) {
        qCritical("Frame Grabber Thread could not be created");
    }
}