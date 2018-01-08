#include "MainWindow.h"

#include <cstddef>

#include <QApplication>
#include <QtOpenGL>
#include <QtGlobal>
#include <QDebug>

#include <LandmarkCoreIncludes.h>

std::string readStyleSheet() {

    std::ifstream stylesheetFile("..\\..\\data\\stylesheet.txt");

    if (!stylesheetFile.is_open()) {
        return "";
    }

    std::stringstream buffer;
    buffer << stylesheetFile.rdbuf();

    std::string result = buffer.str();

    return result;
}


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


    a.setStyleSheet(QString::fromStdString(readStyleSheet()));

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
    w.showMaximized();

    //
    // Qt Main Loop
    //
    return a.exec();
}
