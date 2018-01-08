#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

struct MemoryPool;

class KinectGrabber;
class PointCloudDisplay;

class QLabel;
class QLineEdit;

namespace LandmarkDetector {
    struct FaceModelParameters;
    class  CLNF;
}

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(MemoryPool* memory, LandmarkDetector::FaceModelParameters* detectionParameters,
                        LandmarkDetector::CLNF* detectionModel, QWidget *parent = 0);
    ~MainWindow();

public slots:
    void FrameReady();
    void DisplayFPS(float fps);

    void NormalComputationRequested(bool);
    void NormalComputationForHemisphereRequested(bool);
    void PointCloudFilterRequested(bool);

    void OnFilterParamsChanged();
    void OnDrawNormalsToggled(bool);
    void OnDrawColorsToggled(bool);
    void OnDoFaceTrackingToggled(bool);
    void OnNormalsComputed();
    void OnPointcloudFiltered();
    void OnSnapshotSaved();

    void SnapshotRequested(bool);
    void LoadSnapshotRequested(bool);
    void CreateTextureRequested(bool);
    void OnNewScanSessionRequested(bool);

private:
    void DisplayColorFrame();
    void DisplayDepthFrame();
    void DisplayPointCloud();

    void createActions();
    void createMenus();
    void createToolBar();

    Ui::MainWindow *ui;

    MemoryPool* memory;
    LandmarkDetector::FaceModelParameters* faceTrackingParameters;  // Default params, play around with them!
    LandmarkDetector::CLNF*  faceTrackingModel;


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

    bool drawNormals;
    QAction* loadSnapshotAction;
    QAction* drawNormalsAction;
    QAction* drawColoredPointCloudAction;
    QAction* faceTrackingAction;
    QAction* filterPointCloudAction;
    QAction* computeNormalsAction;
    QAction* computeNormalsForHemisphereAction;
    QAction* saveSnapshotAction;
    QAction* textureGenerationAction;
};

#endif // MAINWINDOW_H
