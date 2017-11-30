#include "KinectGrabber.h"

#include <QtDebug>
#include <QElapsedTimer>
#include <QThread>

#include <LandmarkCoreIncludes.h>
#include <opencv2/opencv.hpp>

#include "FaceTrackingVis.h"
#include "MemoryPool.h"
#include "util.h"
#include "PointCloud.h"

/**
 * Template function for Releasing various resources from the Kinect API
 */
template<class Resource>
void SafeRelease(Resource*& resource) {
    if (resource != NULL) {
        resource->Release();
        resource = NULL;
    }
}

/**
 * @brief FrameGrabberThread   This is the starting point of the Thread, which captures the Kinect Multiframes
 *
 * @param params   An instance of the starting KinectGrabber object is passed to the thread.
 *
 * @return  0 on exit
 */
INT32 FrameGrabberThread(void* params) {
    KinectGrabber* grabber = (KinectGrabber*) params;
    grabber->StartFrameGrabbingLoop();
    return 0;
}

/**
 * @brief KinectGrabber::KinectGrabber  The constructor allocates memory for all buffers.
 */
KinectGrabber::KinectGrabber(FrameBuffer *multiFrameBuffer,
                             LandmarkDetector::CLNF *faceTrackingModel,
                             LandmarkDetector::FaceModelParameters *faceTrackingParameters) :
    faceTrackingModel_(faceTrackingModel),
    faceTrackingParameters_(faceTrackingParameters)
{

    this->multiFrameBuffer = multiFrameBuffer;
   // depthBufferSize = DEPTH_HEIGHT * DEPTH_WIDTH;
   // depthBuffer     = new UINT16[depthBufferSize];

    tmpPositions = new CameraSpacePoint[NUM_DEPTH_PIXELS];
    tmpColors    = new ColorSpacePoint [NUM_DEPTH_PIXELS];

    bodyIndexBufferSize = NUM_DEPTH_PIXELS;
    bodyIndexBuffer     = new UINT8[bodyIndexBufferSize];

    captureNonTrackedBodies = false;
}

/**
 * @brief KinectGrabber::~KinectGrabber  Frees memory and resources on destruction
 */
KinectGrabber::~KinectGrabber()
{
    reader->UnsubscribeMultiSourceFrameArrived(frameHandle);
    sensor->Close();

    SafeRelease(coordinateMapper);
    SafeRelease(reader);
    SafeRelease(sensor);

    // delete [] depthBuffer;
    delete [] bodyIndexBuffer;
    delete [] tmpPositions;
    delete [] tmpColors;
}

/**
 * @brief KinectGrabber::ConnectToKinect   Open Kinect Sensor and FrameReader
 */
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

/**
 * @brief KinectGrabber::StartStream  Create and Start Capture Thread
 */
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

/**
 * @brief KinectGrabber::StartFrameGrabbingLoop  Infinite Loop that waits for Kinect Signals
 *
 * Exits on maxTimeoutTries timeouts with 100ms each.
 */
void KinectGrabber::StartFrameGrabbingLoop() {

    HANDLE handles[] = { (HANDLE)frameHandle };

    int maxTimeoutTries = 30;
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

/**
 * @brief KinectGrabber::ProcessMultiFrame Acquire MultiFrame and process individual elements
 */
void KinectGrabber::ProcessMultiFrame() {
    // Acquire MultiFrame
    hr = reader->AcquireLatestFrame(&multiFrame);
    if (FAILED(hr)) { qCritical("Could not acquire frame"); return; }

    //
    // Process individual components
    //

    bool colorAvailable = ProcessColor();

    // Start facetracking thread after we acquired the color buffer
    FaceTrackingThread* thread = new FaceTrackingThread(multiFrameBuffer->colorBuffer, faceTrackingModel_, faceTrackingParameters_);
    thread->start();

    bool depthAvailable = ProcessDepth();
    bool bodyIndexAvailable = ProcessBodyIndex();

    bool canComputePointCloud = colorAvailable &&
                                depthAvailable &&
                                bodyIndexAvailable;

    if (canComputePointCloud) {

       // Depth buffer is filled here
       hr = coordinateMapper->MapColorFrameToCameraSpace(NUM_DEPTH_PIXELS, multiFrameBuffer->depthBuffer16,
                                                         NUM_COLOR_PIXELS, (CameraSpacePoint*)multiFrameBuffer->colorToCameraMapping);

       if (FAILED(hr)) { qCritical("Could not map from color to camera space"); }

       CreatePointCloud();
    }

    // Wait for facetracking thread to finish
    thread->wait();
    thread->deleteLater();

    // Visualize results
    cv::Mat captured_image = cv::Mat(COLOR_HEIGHT, COLOR_WIDTH, CV_8UC4, multiFrameBuffer->colorBuffer);
    cv::resize(captured_image, captured_image, cv::Size(), 0.6, 0.6);
    FaceTrackingVisualization::visualise_tracking(captured_image, *faceTrackingModel_, *faceTrackingParameters_);

    cv::waitKey(1);

    //
    // Find 3D Points that correspond to the 2D Landmarks
    //
    //  Map landmark points to camera space and do a nearest neighbor search on the resulting point
    //

    CameraSpacePoint* points = (CameraSpacePoint*)multiFrameBuffer->colorToCameraMapping;
    PointCloudHelpers::KDTree tree(3, *multiFrameBuffer->pointCloudBuffer, nanoflann::KDTreeSingleIndexAdaptorParams());
    tree.buildIndex();

    // Landmarks are stored as [x1, ... ,xn, y1, ..., yn]
    cv::Mat_<double>landmarks = faceTrackingModel_->detected_landmarks;
    int numLandmarks = landmarks.rows / 2;
    size_t nearestNeighborIndex = 0;
    float  squaredDistance = -1.0f;

    std::vector<size_t> landmarkIndices;
    for (int i = 0; i < landmarks.rows / 2; ++i) {
        float x = landmarks.at<double>(i);
        float y = landmarks.at<double>(i + numLandmarks);

        // Convert back to image coordinates of the original size
        x /= captured_image.cols;
        y /= captured_image.rows;
        x *= COLOR_WIDTH;
        y *= COLOR_HEIGHT;

        int ix = (int)std::round(x);
        int iy = (int)std::round(y);

        int index = LINEAR_INDEX(iy, ix, COLOR_WIDTH);
        CameraSpacePoint p = points[index];
        size_t result = tree.knnSearch((float*)(&p.X), 1, &nearestNeighborIndex, &squaredDistance);

        if (result == 0) {
            qWarning("No nearest neighbors found");
        } else {
            landmarkIndices.push_back(nearestNeighborIndex);

            // TODO: for visualization only, remove and only store the gathered indices
            multiFrameBuffer->pointCloudBuffer->colors[nearestNeighborIndex] = {0.1f, 1.0f, 0.1f};
        }
    }
    qInfo() << landmarkIndices;

    bool frameReady = canComputePointCloud;
    if (frameReady) { emit FrameReady(); }

    SafeRelease(multiFrame);
}

bool KinectGrabber::ProcessColor() {
    bool succeeded = false;

    hr = multiFrame->get_ColorFrameReference(&colorFrameReference);
    if (FAILED(hr)) { qCritical("No Color Frame"); return succeeded; }

    hr = colorFrameReference->AcquireFrame(&colorFrame);

    if (SUCCEEDED(hr)) {
        hr = colorFrame->CopyConvertedFrameDataToArray(COLOR_BUFFER_SIZE,
                                                       (BYTE*)multiFrameBuffer->colorBuffer,
                                                       ColorImageFormat_Rgba);

        if (SUCCEEDED(hr)) {
            succeeded = true;
        } else {
            qWarning("Could not copy color frame data to buffer");
        }

        SafeRelease(colorFrame);
    } else {
        qWarning("Could not acquire color frame");
    }

    SafeRelease(colorFrameReference);
    return succeeded;
}


/**
 * @brief KinectGrabber::ProcessDepth converts 16bit depth to 8bit and copies the data to the internal buffer
 * @return true on success
 */
bool KinectGrabber::ProcessDepth() {
    bool succeeded = false;

    hr = multiFrame->get_DepthFrameReference(&depthFrameReference);
    if (FAILED(hr)) { qCritical("No Depth Frame"); return succeeded; }

    uint8_t* depthBuffer8Bit = multiFrameBuffer->depthBuffer8;
    uint16_t* depthBuffer = multiFrameBuffer->depthBuffer16;

    hr = depthFrameReference->AcquireFrame(&depthFrame);
    if (SUCCEEDED(hr)) {
        UINT16 minDistance, maxDistance;
        bool minDistanceAvailabe, maxDistanceAvailable;

        minDistanceAvailabe  = SUCCEEDED(depthFrame->get_DepthMinReliableDistance(&minDistance));
        maxDistanceAvailable = SUCCEEDED(depthFrame->get_DepthMaxReliableDistance(&maxDistance));

        hr = depthFrame->CopyFrameDataToArray(NUM_DEPTH_PIXELS, depthBuffer);
        if (SUCCEEDED(hr) && minDistanceAvailabe && maxDistanceAvailable) {
            SafeRelease(depthFrame);
            float scale = 255.0f / (maxDistance - minDistance);
            int numPixels = DEPTH_HEIGHT * DEPTH_WIDTH;

            for (int pixel = 0; pixel < numPixels; ++pixel) {
                INT32 depth = (INT32)depthBuffer[pixel];
                depth -= (INT32)minDistance;
                float val = depth * scale;
                depthBuffer8Bit[pixel] = FloatToUINT8(val);
            }

            succeeded = true;
        } else {
            SafeRelease(depthFrame);
            qWarning("Could not copy depth frame data to buffer");
        }

    } else {
        qWarning("Could not acquire depth frame");
    }

    SafeRelease(depthFrameReference);
    return succeeded;
}

bool KinectGrabber::ProcessBodyIndex()
{
    bool succeeded = false;

    hr = multiFrame->get_BodyIndexFrameReference(&bodyIndexFrameReference);
    if (FAILED(hr)) { qCritical("No Body Index Frame"); return succeeded; }

    hr = bodyIndexFrameReference->AcquireFrame(&bodyIndexFrame);
    if (SUCCEEDED(hr)) {
        hr = bodyIndexFrame->CopyFrameDataToArray(bodyIndexBufferSize, bodyIndexBuffer);
        if (SUCCEEDED(hr)) {
            succeeded = true;
        } else {
            qWarning("Could not copy body index data to internal buffer");
        }
        SafeRelease(bodyIndexFrame);
    } else {
        qWarning("Could not acquire body index frame");
    }

    SafeRelease(bodyIndexFrameReference);
    return succeeded;
}

/**
 * @brief KinectGrabber::CreatePointCloud uses the coordinate mapper of the kinect to gather
 * a colored 3D PointCloud
 * @return
 *
 */

#include <cstdint>
bool KinectGrabber::CreatePointCloud() {
    QElapsedTimer timer;
    timer.start();
    bool succeeded = false;

    // Grab buffers from framebuffer object
    uint32_t* colorBuffer   = multiFrameBuffer->colorBuffer;
    uint16_t* depthBuffer   = multiFrameBuffer->depthBuffer16;
    RGB3f* pointCloudColors = multiFrameBuffer->pointCloudBuffer->colors;
    Vec3f* pointCloudPoints = multiFrameBuffer->pointCloudBuffer->points;
    size_t numPoints = 0;

    // Temp buffers
    // TODO: avoid allocating memory each frame
    // CameraSpacePoint* tmpPositions = new CameraSpacePoint[NUM_DEPTH_PIXELS];
    // ColorSpacePoint*  tmpColors    = new ColorSpacePoint [NUM_DEPTH_PIXELS];

    // int32_t maxFaceX, minFaceX, maxFaceY, minFaceY;
    // minFaceX = INT32_MAX;
    // maxFaceX = INT32_MIN;
    // minFaceY = INT32_MAX;
    // maxFaceY = INT32_MIN;


    hr = coordinateMapper->MapDepthFrameToCameraSpace(NUM_DEPTH_PIXELS, depthBuffer,
                                                      NUM_DEPTH_PIXELS, tmpPositions);
    if (SUCCEEDED(hr)) {
        hr = coordinateMapper->MapDepthFrameToColorSpace(NUM_DEPTH_PIXELS, depthBuffer,
                                                         NUM_DEPTH_PIXELS, tmpColors);
        if (SUCCEEDED(hr)) {

            CameraSpacePoint p;
            ColorSpacePoint c;

//            for (int row = 0; row < DEPTH_HEIGHT; ++row) {
//                for (int col = 0; col < DEPTH_WIDTH; ++col) {
            for (int depthPixel = 0; depthPixel < NUM_DEPTH_PIXELS; ++depthPixel) {

                //int depthPixel = LinearIndex(row, col, DEPTH_WIDTH);
                    p = tmpPositions[depthPixel];



                    UINT8 bodyIndex = bodyIndexBuffer[depthPixel];
                    bool pointBelongsToTrackedBody = (bodyIndex != 0xFF);

                    bool isInvalidMapping = std::isinf(p.X) || std::isnan(p.X) ||
                                            std::isinf(p.Y) || std::isnan(p.Y) ||
                                            std::isinf(p.Z) || std::isnan(p.Z);

                    if (pointBelongsToTrackedBody && !isInvalidMapping) {

                        // Store valid 3D point position
                        //pointCloudPoints[numPoints] = Vec3f(&p.X);
                        pointCloudPoints[numPoints] = {p.X, p.Y, p.Z};

                        //pointCloudPoints[numPoints].X = p.X;
                        //pointCloudPoints[numPoints].Y = p.Y;
                        //pointCloudPoints[numPoints].Z = p.Z;

                        // Try to get color for point
                        c = tmpColors[depthPixel];

                        // Round floating point indices to int
                        int colorIndexCol = (int)(c.X + 0.5f);
                        int colorIndexRow = (int)(c.Y + 0.5f);

                        int colorIndex = LINEAR_INDEX(colorIndexRow, colorIndexCol, COLOR_WIDTH);
                        bool colorIndexInvalid = (colorIndex < 0) || (colorIndex >= NUM_COLOR_PIXELS);

                        // If color mapping is invalid, we just write a gray value
                        if (!colorIndexInvalid) {
                            // if (colorIndexCol > maxFaceX) {
                            //     maxFaceX = colorIndexCol;
                            // } else if (colorIndexCol < minFaceX) {
                            //     minFaceX = colorIndexCol;
                            // }
                            //
                            // if (colorIndexRow > maxFaceY) {
                            //     maxFaceY = colorIndexRow;
                            // } else if (colorIndexRow < minFaceY) {
                            //     minFaceY = colorIndexRow;
                            // }

                            RGBQUAD* rgbx = (RGBQUAD*)(colorBuffer + colorIndex);

                            // colors are swapped here from bgr to rgb for opengl!
                            // pointCloudColors.push_back(RGB3f(rgbx.rgbRed, rgbx.rgbGreen, rgbx.rgbBlue));
                            // pointCloudColors[numPoints] = RGB3f(rgbx->rgbRed, rgbx->rgbGreen, rgbx->rgbBlue);
                            pointCloudColors[numPoints] = {rgbx->rgbBlue  / 255.0f,
                                                           rgbx->rgbGreen / 255.0f,
                                                           rgbx->rgbRed   / 255.0f};
                           // pointCloudColors[numPoints].B = ((float)rgbx->rgbRed)   / 255.0f;
                           // pointCloudColors[numPoints].G = ((float)rgbx->rgbGreen) / 255.0f;
                           // pointCloudColors[numPoints].R = ((float)rgbx->rgbBlue)  / 255.0f;

                        } else {
                            // uint8_t gray[3] = { 100, 100, 100 };
                            // pointCloudColors.push_back(RGB3f(gray));
                            // pointCloudColors[numPoints] = RGB3f(0.6f, 0.6f, 0.6f);
                            pointCloudColors[numPoints] = {0.5f, 0.5f, 0.5f};
                        }

                        numPoints++;
                    }
            }

            PointCloudBuffer* buf = multiFrameBuffer->pointCloudBuffer;
            buf->numPoints = numPoints;
            // buf->minFaceX  = minFaceX;
            // buf->maxFaceX  = maxFaceX;
            // buf->minFaceY  = minFaceY;
            // buf->maxFaceY  = maxFaceY;

            succeeded = true;
        } else {
            qWarning("Coordinate Mapper Error: Could not map from depth to color space");
        }
    } else {
        qWarning("Coordinate Mapper Error: Could not map from depth to camera space");
    }

    // delete [] tmpColors;
    // delete [] tmpPositions;

    qInfo() << "Creating Pointcloud in " << timer.elapsed() << "ms";

    return succeeded;
}

void KinectGrabber::RetrieveTrackedBodiesOnlySettingsChanged(int captureTrackedBodiesOnlyState)
{
    if (captureTrackedBodiesOnlyState == Qt::Unchecked) {
        captureNonTrackedBodies = false;
    } else {
        captureNonTrackedBodies = true;
    }
}


void FaceTrackingThread::run()
{
    QElapsedTimer timer;
    timer.start();
    cv::Mat captured_image = cv::Mat(COLOR_HEIGHT, COLOR_WIDTH, CV_8UC4, colors);
    cv::resize(captured_image, captured_image, cv::Size(), 0.6, 0.6);
    cv::Mat_<uchar> gray;

    cv::cvtColor(captured_image, gray, CV_BGRA2GRAY);

//    PointCloudBuffer* pcbuf = memory->gatherBuffer.pointCloudBuffer;
//    double minFaceX = (double)pcbuf->minFaceX;
//    double minFaceY = (double)pcbuf->minFaceY;
//    double width  = ((double)pcbuf->maxFaceX) - minFaceX;
//    double height = ((double)pcbuf->maxFaceY) - minFaceY;
//    cv::Rect_<double> boundingBox = cv::Rect_<double>(minFaceX, minFaceY, width, height);

 //   qInfo() << minFaceX << minFaceY << width << height;
    // LandmarkDetector::DetectLandmarksInVideo(gray, boundingBox,  *faceTrackingModel, *faceTrackingParameters);
    bool success = LandmarkDetector::DetectLandmarksInVideo(gray, *faceTrackingModel_, *faceTrackingParameters_);

#if 0
    float fx, fy, cx, cy;
    cx = captured_image.cols / 2.0f;
    cy = captured_image.rows / 2.0f;
    fx = 500 * (captured_image.cols / 640.0);
    fy = 500 * (captured_image.rows / 480.0);

    fx = (fx + fy) / 2.0;
    fy = fx;
    FaceTrackingVisualization::visualise_tracking(captured_image, *faceTrackingModel_, *faceTrackingParameters_,
                                                  cv::Point3f(), cv::Point3f(), 0, fx, fy, cx, cy);

    cv::Mat_<double>landmarks = faceTrackingModel_->detected_landmarks;
    int numLandmarks = landmarks.rows / 2;
    if (success) {
        std::vector<size_t> landmarkIndices(numLandmarks);
        for (int i = 0; i < landmarks.rows / 2; ++i) {
            float x = landmarks.at<double>(i);
            float y = landmarks.at<double>(i + numLandmarks);

            qInfo() << x << y;
            // Convert back to original size image coordinates
//            x /= captured_image.cols;
//            y /= captured_image.rows;
//
//            x *= COLOR_WIDTH;
//            y *= COLOR_WIDTH;
//
//            int ix = (int)std::round(x);
//            int iy = (int)std::round(y);
        }
    } else {
        qInfo("Face tracking failed");
    }
#endif

    qInfo() << "Facetracking took " << timer.elapsed() << "ms";
}
