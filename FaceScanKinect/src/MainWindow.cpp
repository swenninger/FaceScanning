#include "MainWindow.h"
#include "ui_mainwindow.h"

#include <QLabel>
#include <QVBoxLayout>

#include <LandmarkCoreIncludes.h>

#include "KinectGrabber.h"
#include "PointCloudDisplay.h"
#include "PointCloud.h"
#include "MemoryPool.h"
#include "FaceTrackingVis.h"

MainWindow::MainWindow(MemoryPool* memory,
                       LandmarkDetector::FaceModelParameters *detectionParameters,
                       LandmarkDetector::CLNF *detectionModel,
                       QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow),
    faceTrackingParameters(detectionParameters),
    faceTrackingModel(detectionModel)
{
    this->memory = memory;

    drawNormals = true;

    ui->setupUi(this);

    createMenus();
    createToolBar();

    kinectGrabber = new KinectGrabber(&memory->gatherBuffer, faceTrackingModel, faceTrackingParameters);
    QObject::connect(kinectGrabber, SIGNAL(FrameReady()), this, SLOT(FrameReady()));

    // Color Frame Display
    colorDisplay = new QLabel();
    colorDisplay->setMinimumSize(200, 200);

    // Depth Frame Display
    depthDisplay = new QLabel();
    depthDisplay->setMinimumSize(200, 200);

    // PointCloud Display
    pointCloudDisplay = new PointCloudDisplay();
    pointCloudDisplay->setMinimumSize(200, 200);

    // Second PointCloud Display for inspecting snapshots and
    // visually inspecting filter and normal algorithms
    inspectionPointCloudDisplay = new PointCloudDisplay();
    inspectionPointCloudDisplay->setMinimumSize(200, 200);

    // Widget that stores all settings and Buttons
    QWidget* settingsBox = new QWidget(this);
    QVBoxLayout* layout  = new QVBoxLayout(settingsBox);
    layout->setSpacing(2);
    settingsBox->setLayout(layout);

    // Header
    layout->addWidget(new QLabel("PointCloud Settings"));

    //
    // Button for testing texture generation
    //
    QPushButton* createTextureButton = new QPushButton("Create Texture");
    createTextureButton->setMaximumWidth(200);
    layout->addWidget(createTextureButton);
    QObject::connect(createTextureButton, SIGNAL(clicked(bool)), this, SLOT(CreateTextureRequested(bool)));

    layout->addWidget(new QLabel("Pointcloud Filter Settings"));

    //
    // Inputs for controlling the pointcloud filter algorithm
    //
    numNeighborsLineEdit = new QLineEdit("numNeighbors");
    numNeighborsLineEdit->setToolTip("Specify the number of neighbors, that is used for filtering the pointcloud");
    numNeighborsLineEdit->setText("25");
    numNeighborsLineEdit->setMaximumWidth(200);
    QIntValidator* intValidator = new QIntValidator(5, 200);
    numNeighborsLineEdit->setValidator(intValidator);
    QObject::connect(numNeighborsLineEdit, SIGNAL(editingFinished()), this, SLOT(OnFilterParamsChanged()));
    layout->addWidget(numNeighborsLineEdit);

    stddevMultiplierLineEdit = new QLineEdit("stddevMultiplier");
    stddevMultiplierLineEdit->setToolTip("Specify the amount of standard deviations, a point is allowed to be away from the mean distance to its neighbors");
    stddevMultiplierLineEdit->setText("1.0");
    stddevMultiplierLineEdit->setMaximumWidth(200);
    QDoubleValidator* doubleValidator = new QDoubleValidator(0.01, 20.0, 3);
    stddevMultiplierLineEdit->setValidator(doubleValidator);
    QObject::connect(stddevMultiplierLineEdit, SIGNAL(editingFinished()), this, SLOT(OnFilterParamsChanged()));
    layout->addWidget(stddevMultiplierLineEdit);

    //
    // Set widget positions in the grid layout
    //
    ui->gridLayout->addWidget(depthDisplay,                0, 0, 1, 1);
    ui->gridLayout->addWidget(colorDisplay,                1, 0, 1, 1);
    ui->gridLayout->addWidget(pointCloudDisplay,           2, 0, 1, 1);
    ui->gridLayout->addWidget(settingsBox,                 0, 1, 1, 3);
    ui->gridLayout->addWidget(inspectionPointCloudDisplay, 2, 1, 1, 1);

    normalComputationRequested = false;
    pointCloudFilterRequested = false;
    snapshotRequested = false;

    // Start Kinect Streaming
    kinectGrabber->ConnectToKinect();
    kinectGrabber->StartStream();
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::createMenus() {
    QStyle* currentStyle = QApplication::style();

    loadSnapshotAction = new QAction("Load Snapshot");
    loadSnapshotAction->setShortcut(QKeySequence(tr("Ctrl+O")));
    connect(loadSnapshotAction, &QAction::triggered, this, &MainWindow::LoadSnapshotRequested);

    drawNormalsAction = new QAction("Draw Normals");
    drawNormalsAction->setShortcut(QKeySequence(tr("Ctrl+N")));
    drawNormalsAction->setCheckable(true);
    drawNormalsAction->setChecked(drawNormals);
    connect(drawNormalsAction, &QAction::triggered, this, &MainWindow::OnDrawNormalsToggled);

    drawColoredPointCloudAction = new QAction("Draw colored PointCloud");
    drawColoredPointCloudAction->setShortcut(QKeySequence(tr("Ctrl+C")));
    drawColoredPointCloudAction->setCheckable(true);
    drawColoredPointCloudAction->setChecked(true);
    connect(drawColoredPointCloudAction, &QAction::triggered, this, &MainWindow::OnDrawColorsToggled);

    faceTrackingAction = new QAction("Track Faces");
    faceTrackingAction->setShortcut(QKeySequence(tr("Ctrl+F")));
    faceTrackingAction->setCheckable(true);
    faceTrackingAction->setChecked(true);
    connect(faceTrackingAction, &QAction::triggered, this, &MainWindow::OnDoFaceTrackingToggled);

    filterPointCloudAction = new QAction("Filter Pointcloud");
    connect(filterPointCloudAction, &QAction::triggered, this, &MainWindow::PointCloudFilterRequested);

    computeNormalsAction = new QAction("Compute Normals");
    connect(computeNormalsAction, &QAction::triggered, this, &MainWindow::NormalComputationRequested);

    computeNormalsForHemisphereAction = new QAction("Compute Normals");
    connect(computeNormalsForHemisphereAction, &QAction::triggered, this, &MainWindow::NormalComputationForHemisphereRequested);

    QMenu* fileMenu = ui->menuBar->addMenu("File");
    fileMenu->addAction(loadSnapshotAction);

    QMenu* viewMenu = ui->menuBar->addMenu("View");
    viewMenu->addAction(drawNormalsAction);
    viewMenu->addAction(drawColoredPointCloudAction);

    QMenu* toolsMenu = ui->menuBar->addMenu("Tools");
    toolsMenu->addAction(faceTrackingAction);
    toolsMenu->addSeparator();
    toolsMenu->addAction(filterPointCloudAction);
    toolsMenu->addAction(computeNormalsAction);
    toolsMenu->addAction(computeNormalsForHemisphereAction);
}

void MainWindow::createToolBar() {
    QStyle* currentStyle = QApplication::style();

    drawNormalsActionFromToolBar = new QAction(QIcon(":/icons/data/icons/raw-svg/solid/external-link-alt.svg"), "Draw Normals");
    drawNormalsActionFromToolBar->setCheckable(true);
    drawNormalsActionFromToolBar->setChecked(drawNormals);
    connect(drawNormalsActionFromToolBar, &QAction::triggered, this, &MainWindow::OnDrawNormalsToggled);

    faceTrackingActionFromToolBar = new QAction(QIcon(":/icons/data/icons/raw-svg/solid/eye.svg"), "Do Facetracking");
    faceTrackingActionFromToolBar->setCheckable(true);
    faceTrackingActionFromToolBar->setChecked(true);
    connect(faceTrackingActionFromToolBar, &QAction::triggered, this, &MainWindow::OnDoFaceTrackingToggled);

    saveSnapshotActionFromToolBar = new QAction(QIcon(":/icons/data/icons/raw-svg/solid/save.svg"), "Save Snapshot");
    saveSnapshotActionFromToolBar->setShortcut(QKeySequence(tr("Ctrl+S")));
    connect(saveSnapshotActionFromToolBar, &QAction::triggered, this, &MainWindow::SnapshotRequested);


    ui->mainToolBar->addAction(drawNormalsActionFromToolBar);
    ui->mainToolBar->addAction(faceTrackingActionFromToolBar);
    ui->mainToolBar->addSeparator();
    ui->mainToolBar->addAction(saveSnapshotActionFromToolBar);
}

void MainWindow::FrameReady()
{
    DisplayDepthFrame();

    // DisplayColorFrame();
    DisplayPointCloud();

    if (normalComputationRequested) {
        CopyPointCloudBuffer(memory->gatherBuffer.pointCloudBuffer, &memory->inspectionBuffer);
        PointCloudHelpers::CreateAndStartNormalWorker(&memory->inspectionBuffer, this);
        normalComputationRequested = false;
    }

    if (pointCloudFilterRequested) {
        CopyPointCloudBuffer(memory->gatherBuffer.pointCloudBuffer, &memory->inspectionBuffer);
        PointCloudHelpers::CreateAndStartFilterWorker(&memory->inspectionBuffer, &memory->filterBuffer, this);
        pointCloudFilterRequested = false;
    }

    if (snapshotRequested) {
        CopyFrameBuffer(&memory->gatherBuffer, &memory->snapshotBuffer);
        PointCloudHelpers::SaveSnapshot(&memory->snapshotBuffer);

        // inspectionPointCloudDisplay->SetData(memory->snapshotBuffer.pointCloudBuffer, true /* with normals */);

        snapshotRequested = false;
    }
}

void MainWindow::DisplayColorFrame()
{
    int height = colorDisplay->size().height();
    QPixmap pixmap = QPixmap::fromImage(QImage((uchar*)memory->gatherBuffer.colorBuffer,
                                               COLOR_WIDTH,
                                               COLOR_HEIGHT,
                                               QImage::Format_RGBA8888));
    colorDisplay->setPixmap(pixmap.scaledToHeight(height));
}

void MainWindow::DisplayDepthFrame()
{
    int height = depthDisplay->size().height();

    QPixmap pixmap = QPixmap::fromImage(QImage((uchar*)memory->gatherBuffer.depthBuffer8,
                                               DEPTH_WIDTH,
                                               DEPTH_HEIGHT,
                                               QImage::Format_Grayscale8));
    depthDisplay->setPixmap(pixmap.scaledToHeight(height));
}

void MainWindow::DisplayPointCloud()
{
    pointCloudDisplay->SetData(memory->gatherBuffer.pointCloudBuffer);
}

void MainWindow::DisplayFPS(float fps)
{
    ui->statusBar->showMessage(QString::number(fps));
}

void MainWindow::OnFilterParamsChanged()
{
    int   numNeighbors     = numNeighborsLineEdit->text().toInt();
    float stddevMultiplier = stddevMultiplierLineEdit->text().toFloat();

    PointCloudHelpers::CreateAndStartFilterWorker(&memory->inspectionBuffer, &memory->filterBuffer,
                                                  this, numNeighbors, stddevMultiplier);
}

void MainWindow::OnDrawNormalsToggled(bool checked)
{
    drawNormalsAction->setChecked(checked);
    drawNormalsActionFromToolBar->setChecked(checked);

    inspectionPointCloudDisplay->Redraw(checked);
}

void MainWindow::OnDrawColorsToggled(bool checked)
{
    pointCloudDisplay->DrawColoredPointcloud(checked);
    inspectionPointCloudDisplay->DrawColoredPointcloud(checked);
}

void MainWindow::OnDoFaceTrackingToggled(bool checked)
{   
    faceTrackingAction->setChecked(checked);
    faceTrackingActionFromToolBar->setChecked(checked);

    kinectGrabber->ToggleFaceTracking();
}

void MainWindow::OnNormalsComputed()
{
    inspectionPointCloudDisplay->SetData(&memory->inspectionBuffer, true /* data has normals */);
}

void MainWindow::OnPointcloudFiltered()
{
    inspectionPointCloudDisplay->SetData(&memory->filterBuffer);
}

void MainWindow::SnapshotRequested(bool)
{
    snapshotRequested = true;
}

void MainWindow::LoadSnapshotRequested(bool)
{
    QString loadFileName = QFileDialog::getOpenFileName(this, "Select Pointcloud File to read", "..\\..\\data\\", "Pointcloud Files(*.pc)", nullptr, QFileDialog::DontUseNativeDialog);

    if (loadFileName.isNull() || loadFileName.isEmpty()) {
        return;
    }

    PointCloudHelpers::LoadSnapshot(loadFileName.toStdString().c_str(), memory->snapshotBuffer.pointCloudBuffer);
    inspectionPointCloudDisplay->SetData(memory->snapshotBuffer.pointCloudBuffer, true /* with normals */);
}

#include "TextureDisplay.h"

void MainWindow::CreateTextureRequested(bool)
{
    TextureDisplay* window = new TextureDisplay(kinectGrabber->GetCoordinateMapper());
    //QWidget* window = new QWidget();
    //window->setMinimumSize(200,200);


    qInfo() << "Calling show on texture display";
    window->show();

}

void MainWindow::NormalComputationRequested(bool)
{
    // Sets the flag and waits for next frame to come in
    normalComputationRequested = true;
}
void MainWindow::NormalComputationForHemisphereRequested(bool)
{
    PointCloudHelpers::GenerateRandomHemiSphere(&memory->inspectionBuffer, 60000);
    PointCloudHelpers::CreateAndStartNormalWorker(&memory->inspectionBuffer, this);
}

void MainWindow::PointCloudFilterRequested(bool)
{
    pointCloudFilterRequested = true;
}
