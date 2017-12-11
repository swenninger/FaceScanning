QT       += core gui opengl

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TEMPLATE = app
CONFIG += c++11
#CONFIG -= app_bundle
#CONFIG -= qt

SOURCES += main.cpp

# OpenCV 3
# INCLUDEPATH += $$PWD/../../OpenFace-master\lib\3rdParty\OpenCV3.1\include
# LIBS += -L"$$PWD/../../OpenFace-master\lib\3rdParty\OpenCV3.1\x64\v140\lib" -lopencv_world310

# OpenCV 2
INCLUDEPATH += "C:/opencv/build/include"
LIBS += -L"C:\opencv\build\x64\vc14\lib" -lopencv_core2413 -lopencv_highgui2413 -lopencv_imgproc2413 -lopencv_objdetect2413
# LIBS += "C:\opencv\build\x64\vc14\lib\opencv_core2413.lib"
# LIBS += "C:\opencv\build\x64\vc14\lib\opencv_highgui2413.lib"
# LIBS += "C:\opencv\build\x64\vc14\lib\opencv_imgproc2413.lib"
# LIBS += "C:\opencv\build\x64\vc14\lib\opencv_objdetect2413.lib"
