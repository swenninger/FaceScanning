#include "MainWindow.h"

#include <cstddef>

#include <QApplication>
#include <QtOpenGL>
#include <QtGlobal>

#include <LandmarkCoreIncludes.h>


/*
 * Declare types to Qt's metaprogramming system
 */
Q_DECLARE_METATYPE(size_t)

#include "MemoryPool.h"

int main(int argc, char *argv[])
{

    /*
     * Register types, so that they can be used in Qt's Signal/Slot - Mechanism
     */
    qRegisterMetaType<size_t>("size_t");

    /*
     * Set the Format for OpenGL.
     *
     *      We want to have a depthbuffer for depth testing
     */
    QSurfaceFormat format;

    format.setDepthBufferSize(24);
    format.setMajorVersion(4);
    format.setMinorVersion(0);

    QSurfaceFormat::setDefaultFormat(format);


    //
    // Qt Application init
    //
    QApplication a(argc, argv);

    //
    // Create Buffers to store frames
    //
    MemoryPool memory;

    //
    // Load up FaceTracking Model
    //
    LandmarkDetector::FaceModelParameters faceTrackingParameters;  // Default params, play around with them!
    faceTrackingParameters.track_gaze = false;
    faceTrackingParameters.model_location = "../../data/model/main_clnf_general.txt";
    LandmarkDetector::CLNF faceTrackingModel(faceTrackingParameters.model_location);

    //
    // Main GUI Element
    //
    MainWindow w(&memory, &faceTrackingParameters, &faceTrackingModel);
    w.show();

    //
    // Qt Main Loop
    //
    return a.exec();
}
