#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

#include "KinectGrabber.h"

class PointCloudDisplay;

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

public slots:
    void DisplayColorFrame(uchar* colorBuffer);
    void DisplayDepthFrame(uchar* depthBuffer);
    void DisplayFPS(float fps);
    void DisplayPointCloud(CameraSpacePoint* p, RGBQUAD* c, size_t size);

private:
    Ui::MainWindow *ui;

    KinectGrabber* kinectGrabber;

    QLabel* colorDisplay;
    QLabel* depthDisplay;
    PointCloudDisplay* pointCloudDisplay;
};

#endif // MAINWINDOW_H
