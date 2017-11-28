#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

struct MemoryPool;

class KinectGrabber;
class PointCloudDisplay;

class QLabel;
class QLineEdit;

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(MemoryPool* memory, QWidget *parent = 0);
    ~MainWindow();

public slots:
    void FrameReady();
    void DisplayFPS(float fps);

    void NormalComputationRequested(bool);
    void NormalComputationForHemisphereRequested(bool);
    void PointCloudFilterRequested(bool);

    void OnFilterParamsChanged();
    void OnDrawNormalsToggled(bool);

    void OnNormalsComputed();
    void OnPointcloudFiltered();

    void SnapshotRequested(bool);
    void LoadSnapshotRequested(bool);

private:
    void DisplayColorFrame();
    void DisplayDepthFrame();
    void DisplayPointCloud();

    Ui::MainWindow *ui;

    MemoryPool* memory;
    KinectGrabber* kinectGrabber;

    QLabel* colorDisplay;
    QLabel* depthDisplay;
    PointCloudDisplay* pointCloudDisplay;
    PointCloudDisplay* inspectionPointCloudDisplay;

    QLineEdit* numNeighborsLineEdit;
    QLineEdit* stddevMultiplierLineEdit ;

    bool normalComputationRequested;
    bool pointCloudFilterRequested;
    bool snapshotRequested;
};

#endif // MAINWINDOW_H
