#-------------------------------------------------
#
# Project created by QtCreator 2017-11-22T14:14:40
#
#-------------------------------------------------

QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = SequenceRecorder
TEMPLATE = app

# The following define makes your compiler emit warnings if you use
# any feature of Qt which as been marked as deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if you use deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0


SOURCES += main.cpp\
        mainwindow.cpp\
        KinectGrabber.cpp \

HEADERS  += mainwindow.h\
         KinectGrabber.h\
         util.h\

FORMS    += mainwindow.ui

# Add Kinect SDK to Dependencies
KINECT_LIB_FOLDER = 'C:/Program Files/Microsoft SDKs/Kinect/v2.0_1409/Lib/x64/'
LIBS += -L$$KINECT_LIB_FOLDER  -lKinect20

INCLUDEPATH += "C:/Program Files/Microsoft SDKs/Kinect/v2.0_1409/inc"
DEPENDPATH  += "C:/Program Files/Microsoft SDKs/Kinect/v2.0_1409/inc"

# Face Tracker
#INCLUDEPATH += "$$PWD/../FaceTrackerQt/include"
#LIBS += "$$PWD/../FaceTrackerQt/build/debug/FaceTrackerQt.lib"

INCLUDEPATH += "$$PWD/../../OpenFace-master\lib\3rdParty\dlib\include"
INCLUDEPATH += "$$PWD/../../OpenFace-master\lib\3rdParty\tbb\include"
INCLUDEPATH += "$$PWD/../../OpenFace-master/lib/local/LandmarkDetector/include"
LIBS += -L"$$PWD/../../OpenFace-master/x64/Release" -lLandmarkDetector -ldlib
LIBS += -L"$$PWD/../../OpenFace-master\lib\3rdParty\tbb\lib\x64\v140" -ltbb
LIBS += -L"$$PWD/../../OpenFace-master\lib\3rdParty\boost\x64\v140\lib" -llibboost_filesystem-vc140-mt-1_60

# OpenCV
#INCLUDEPATH += "C:/opencv/build/include"
INCLUDEPATH += $$PWD/../../OpenFace-master\lib\3rdParty\OpenCV3.1\include
LIBS += -L"$$PWD/../../OpenFace-master\lib\3rdParty\OpenCV3.1\x64\v140\lib" -lopencv_world310
# -lopencv_core2413  \
# -lopencv_highgui2413 \
# -lopencv_imgproc2413 \
# -lopencv_objdetect2413 \
# -lopencv_calib3d2413
