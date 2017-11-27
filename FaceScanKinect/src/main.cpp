#include "MainWindow.h"
#include <QApplication>
#include <QtOpenGL>

#include <cstddef>

/*
 * Declare types to Qt's metaprogramming system
 */
Q_DECLARE_METATYPE(size_t)
Q_DECLARE_METATYPE(CapturedFrame)

#include "MemoryPool.h"

int main(int argc, char *argv[])
{

    /*
     * Register types, so that they can be used in Qt's Signal/Slot - Mechanism
     */
    qRegisterMetaType<size_t>("size_t");
    qRegisterMetaType<CapturedFrame>("CapturedFrame");

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
    // Main GUI Element
    //
    MainWindow w(&memory);
    w.show();


    //
    // Qt Main Loop
    //
    return a.exec();
}
