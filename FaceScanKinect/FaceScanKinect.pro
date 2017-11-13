#-------------------------------------------------
#
# Project created by QtCreator 2017-10-19T20:16:44
#
#-------------------------------------------------

QT       += core gui opengl

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = FaceScanKinect
TEMPLATE = app

# The following define makes your compiler emit warnings if you use
# any feature of Qt which has been marked as deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if you use deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0


SOURCES += \
    src/KinectGrabber.cpp \
    src/main.cpp \
    src/MainWindow.cpp \
    src/PointCloudDisplay.cpp

HEADERS += \
    src/KinectGrabber.h \
    src/MainWindow.h \
    src/PointCloudDisplay.h \
    src/util.h \
    src/nanoflann.hpp

FORMS += \
    mainwindow.ui

# Add Kinect SDK to Dependencies
KINECT_LIB_FOLDER = 'C:/Program Files/Microsoft SDKs/Kinect/v2.0_1409/Lib/x64/'
LIBS += -L$$KINECT_LIB_FOLDER  -lKinect20

INCLUDEPATH += "C:/Program Files/Microsoft SDKs/Kinect/v2.0_1409/inc"
DEPENDPATH  += "C:/Program Files/Microsoft SDKs/Kinect/v2.0_1409/inc"

# Eigen Library
INCLUDEPATH += "C:/Eigen/Eigen"
