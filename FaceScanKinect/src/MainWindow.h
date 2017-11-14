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
    void FrameReady(CapturedFrame frame);
    void DisplayFPS(float fps);

    void PointCloudSaveRequested(bool);
    void PointCloudLoadRequested(bool);
    void NormalComputationRequested(bool);
    void NormalComputationForHemisphereRequested(bool);

    void OnFileDestinationChosen();

signals:
    void FileDestinationChosen();

private:
    void DisplayColorFrame(uchar* colorBuffer);
    void DisplayDepthFrame(uchar* depthBuffer);

    Ui::MainWindow *ui;

    KinectGrabber* kinectGrabber;

    QLabel* colorDisplay;
    QLabel* depthDisplay;
    PointCloudDisplay* pointCloudDisplay;
    PointCloudDisplay* inspectionPointCloudDisplay;

    PointCloud inspectedPointCloud;

    bool pointCloudSaveRequested;
    bool pointCloudSaveDone;
    PointCloud pointCloudBuffer;
    QString saveFilename;

    bool normalComputationRequested;
};

#endif // MAINWINDOW_H
