#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

#include "util.h"
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
    void DisplayPointCloud(Vec3f* p, RGB3f* c, int size);
    void PointCloudSaveRequested(bool);
    void PointCloudLoadRequested(bool);


private:
    Ui::MainWindow *ui;

    KinectGrabber* kinectGrabber;

    QLabel* colorDisplay;
    QLabel* depthDisplay;
    PointCloudDisplay* pointCloudDisplay;
    PointCloudDisplay* inspectionPointCloudDisplay;

    PointCloud inspectedPointCloud;

    int count;
    bool pointCloudSaveRequested;
    bool pointCloudSaveDone;

    QString saveFilename;
};

#endif // MAINWINDOW_H
