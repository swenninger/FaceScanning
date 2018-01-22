#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

struct MemoryPool;

class QLabel;
class QLineEdit;

class KinectGrabber;
class PointCloudDisplay;
class TextureDisplay;


namespace LandmarkDetector {
    struct FaceModelParameters;
    class  CLNF;
}

namespace Ui {
    class MainWindow;
}

class OpenCVWebcamGrabber;
class SnapshotGrid;

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
    void OnSnapshotSaved(QString metaFileLocation);

    void SnapshotRequested(bool);
    void LoadSnapshotRequested(bool);
    void CreateTextureRequested(bool);
    void OnNewScanSessionRequested(bool);
    void MeshCreationRequested(bool);

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
    OpenCVWebcamGrabber* openCVGrabber;

    QLabel* colorDisplay;
    QLabel* depthDisplay;
    PointCloudDisplay* pointCloudDisplay;
    PointCloudDisplay* inspectionPointCloudDisplay;

    TextureDisplay* textureDisplay;

    SnapshotGrid* snapshotGrid;

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
    QAction* createMeshesAction;

    QLabel* scanSessionStatus;
};

#endif // MAINWINDOW_H
