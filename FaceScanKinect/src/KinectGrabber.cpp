#include "KinectGrabber.h"
#include <QtDebug>

#include <iostream>

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

    pointCloudPoints = std::vector<CameraSpacePoint>(depthBufferSize / 2);
    pointCloudColors = std::vector<RGBQUAD>(depthBufferSize / 2);

    timer = new QElapsedTimer();
}

KinectGrabber::~KinectGrabber()
{
    reader->UnsubscribeMultiSourceFrameArrived(frameHandle);
    sensor->Close();

    SafeRelease(coordinateMapper);
    SafeRelease(reader);
    SafeRelease(sensor);

    delete [] colorBuffer;
    delete [] depthBuffer;
    delete [] depthBuffer8Bit;
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
    hr = reader->AcquireLatestFrame(&multiFrame);
    if (FAILED(hr)) { qCritical("Could not acquire frame"); return; }

    float fps = 1000.0f / timer->elapsed();
    emit FPSStatusMessage(fps);
    timer->restart();

    // Process individual components
    bool colorAvailable = ProcessColor();
    bool depthAvailable = ProcessDepth();

    if (colorAvailable && depthAvailable) { CreatePointCloud(); }

    SafeRelease(multiFrame);
}

bool KinectGrabber::ProcessColor() {
    bool succeeded = false;

    hr = multiFrame->get_ColorFrameReference(&colorFrameReference);
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

    hr = multiFrame->get_DepthFrameReference(&depthFrameReference);
    if (FAILED(hr)) { qCritical("No Depth Frame"); return false; }

    hr = depthFrameReference->AcquireFrame(&depthFrame);
    if (SUCCEEDED(hr)) {
        UINT16 minDistance, maxDistance;
        bool minDistanceAvailabe, maxDistanceAvailable;

        minDistanceAvailabe  = SUCCEEDED(depthFrame->get_DepthMinReliableDistance(&minDistance));
        maxDistanceAvailable = SUCCEEDED(depthFrame->get_DepthMaxReliableDistance(&maxDistance));
        // hr = depthFrame->AccessUnderlyingBuffer(&depthBufferSize, &depthBuffer);
        hr = depthFrame->CopyFrameDataToArray(depthBufferSize, depthBuffer);
        if (SUCCEEDED(hr) && minDistanceAvailabe && maxDistanceAvailable) {
            float scale = 255.0f / (maxDistance - minDistance);
            int numPixels = DEPTH_HEIGHT * DEPTH_WIDTH;

            for (int pixel = 0; pixel < numPixels; ++pixel) {
                INT32 depth = (INT32)depthBuffer[pixel];
                depth -= (INT32)minDistance;
                float val = depth * scale;
                depthBuffer8Bit[pixel] = FloatToUINT8(val);
            }

            succeeded = true;
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

inline int LinearIndex(int row, int col, int width) {
    int result = row * width + col;
    return result;
}


#include <fstream>
bool KinectGrabber::CreatePointCloud() {
    bool succeeded = false;

    CameraSpacePoint* tmpPositions = new CameraSpacePoint[depthBufferSize];
    ColorSpacePoint*  tmpColors    = new ColorSpacePoint [depthBufferSize];

    hr = coordinateMapper->MapDepthFrameToCameraSpace(depthBufferSize, depthBuffer,
                                                      depthBufferSize, tmpPositions);

    qCritical("mapping");
    if (SUCCEEDED(hr)) {
        hr = coordinateMapper->MapDepthFrameToColorSpace(depthBufferSize, depthBuffer,
                                                         depthBufferSize, tmpColors);
        if (SUCCEEDED(hr)) {
            CameraSpacePoint p;
            ColorSpacePoint c;
            for (int row = 0; row < DEPTH_HEIGHT; ++row) {
                for (int col = 0; col < DEPTH_WIDTH; ++col) {
                    int index = LinearIndex(row, col, DEPTH_WIDTH);
                    p = tmpPositions[index];

                    bool isInvalidMapping = qIsInf(p.X) || qIsInf(p.Y) || qIsInf(p.Z);
                    if (!isInvalidMapping) {
                        c = tmpColors[index];

                        int colorIndexCol = (int)(c.X + 0.5f);
                        int colorIndexRow = (int)(c.Y + 0.5f);

                        int colorIndex = LinearIndex(colorIndexRow, colorIndexCol, COLOR_WIDTH);
                        bool colorIndexInRange = colorIndex > 0 && colorIndex < COLOR_WIDTH * COLOR_HEIGHT;
                        if (colorIndexInRange) {
                            RGBQUAD& rgbx = colorBuffer[colorIndex];
                            pointCloudPoints.push_back(p);
                            pointCloudColors.push_back(rgbx);

                            Q_ASSERT(pointCloudColors.size() == pointCloudPoints.size());
                            emit PointCloudDataAvailable(&pointCloudPoints[0], &pointCloudColors[0], pointCloudPoints.size());
                        }
                    }
                }
            }

#if 0
            QString xs, ys, zs;

            CameraSpacePoint p;
            ColorSpacePoint c;

            // Fill up buffers

            std::ofstream points;
            points.open("points.txt");

            std::ofstream colors;
            colors.open("colors.txt");

            for (int row = 0; row < DEPTH_HEIGHT; ++row) {
                for (int col = 0; col < DEPTH_WIDTH; ++col) {
                    int index = LinearIndex(row, col, DEPTH_WIDTH);
                    p = pointPositions[index];

                    if (!qIsInf(p.X)) {

                        c = pointColors[index];

                        int colorIndexCol = (int)(c.X + 0.5f);
                        int colorIndexRow = (int)(c.Y + 0.5f);

                        int colorIndex = LinearIndex(colorIndexRow, colorIndexCol, COLOR_WIDTH);

                        if (colorIndex > 0 && colorIndex < COLOR_WIDTH * COLOR_HEIGHT) {
                            points << p.X << ", " << p.Y << ", " << p.Z << std::endl;
                            colors << "(" << (int)colorBuffer[colorIndex].rgbBlue  << ", "
                                          << (int)colorBuffer[colorIndex].rgbGreen << ", "
                                          << (int)colorBuffer[colorIndex].rgbRed   << ")" << std::endl;
                        }

                    }
                }
            }

            points.close();
            colors.close();
#endif
            succeeded = true;
        } else {
            //TODO: logging
        }
    } else {
        // TODO: Logging
    }

    delete [] tmpColors;
    delete [] tmpPositions;

    return succeeded;
}

void KinectGrabber::ConnectToKinect() {
    // Get Sensor
    hr = GetDefaultKinectSensor(&sensor);
    if (FAILED(hr)) { qCritical("Default KinectSensor could not be retrieved."); return; }

    // Get Coordinate Mapper
    hr = sensor->get_CoordinateMapper(&coordinateMapper);
    if (FAILED(hr)) { qCritical("Coordinate Mapper could not be retrieved."); return; }

    // Open Sensor
    hr = sensor->Open();
    if (FAILED(hr)) { qCritical("KinectSensor could not be opened."); return; }

    // Get Frame Reader
    hr = sensor->OpenMultiSourceFrameReader(FrameSourceTypes_Depth |
                                            FrameSourceTypes_Color |
                                            FrameSourceTypes_BodyIndex,
                                            &reader);
    if (FAILED(hr)) { qCritical("MultiSourceFrameReader could not be opened."); return; }

}

void KinectGrabber::StartStream() {
    hr = reader->SubscribeMultiSourceFrameArrived(&frameHandle);
    if (FAILED(hr)) { qCritical("Failed to create Stream Handle. Cannot Start Stream"); return; }

    timer->start();

    // Start thread and pass this object as thread parameter
    frameGrabberThreadHandle =
            CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)&FrameGrabberThread, this, 0, &frameGrabberThreadID);

    if (frameGrabberThreadHandle == NULL) {
        qCritical("Frame Grabber Thread could not be created");
    }
}
