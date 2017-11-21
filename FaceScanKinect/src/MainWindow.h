#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

#include "util.h"
#include "KinectGrabber.h"

class PointCloudDisplay;
class QLineEdit;

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
    void PointCloudFilterRequested(bool);

    void OnFileDestinationChosen();
    void OnFilterParamsChanged();

    void SnapshotRequested(bool);
    void LoadSnapshotRequested(bool);

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

    QLineEdit* numNeighborsLineEdit;
    QLineEdit* stddevMultiplierLineEdit ;

    PointCloud inspectedPointCloud;

    bool pointCloudSaveRequested;
    bool pointCloudSaveDone;
    PointCloud pointCloudBuffer;
    QString saveFilename;

    bool normalComputationRequested;
    bool pointCloudFilterRequested;
    bool snapshotRequested;
};

#endif // MAINWINDOW_H
