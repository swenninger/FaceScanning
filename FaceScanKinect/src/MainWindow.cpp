#include "MainWindow.h"
#include "ui_mainwindow.h"

#include <QLabel>
#include <QVBoxLayout>

#include "KinectGrabber.h"
#include "PointCloudDisplay.h"
#include "PointCloud.h"
#include "MemoryPool.h"

MainWindow::MainWindow(MemoryPool* memory, QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    this->memory = memory;

    ui->setupUi(this);
    kinectGrabber = new KinectGrabber(&memory->gatherBuffer);
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
    // Checkbox for toggling if we display/gather points that do not belong to tracked bodies
    //
    QCheckBox* drawNonTrackedPoints = new QCheckBox("Draw non-tracked points");
    layout->addWidget(drawNonTrackedPoints);
    QObject::connect(drawNonTrackedPoints, SIGNAL(stateChanged(int)), kinectGrabber, SLOT(RetrieveTrackedBodiesOnlySettingsChanged(int)));
    drawNonTrackedPoints->setChecked(false);

    //
    // Checkbox for toggling if we display point colors
    //
    QCheckBox* drawColoredPointcloud = new QCheckBox("Draw colored points");
    layout->addWidget(drawColoredPointcloud);
    QObject::connect(drawColoredPointcloud, SIGNAL(stateChanged(int)), pointCloudDisplay, SLOT(ColoredPointsSettingChanged(int)));
    QObject::connect(drawColoredPointcloud, SIGNAL(stateChanged(int)), inspectionPointCloudDisplay, SLOT(ColoredPointsSettingChanged(int)));
    drawColoredPointcloud->setChecked(true);

    //
    // Button for computing the normals of the currently captured Pointcloud and displaying them in second window
    //
    QPushButton* computeNormalsButton = new QPushButton("Compute Normals");
    computeNormalsButton->setMaximumWidth(200);
    layout->addWidget(computeNormalsButton);
    QObject::connect(computeNormalsButton, SIGNAL(clicked(bool)), this, SLOT(NormalComputationRequested(bool)));

    //
    // Button for generating a hemisphere and computing + displaying the normals for it
    // (for visually verifying the normal generation algorithm)
    //
    QPushButton* computeNormalsForHemiSphereButton = new QPushButton("Compute Normals for Hemisphere");
    computeNormalsForHemiSphereButton->setMaximumWidth(200);
    layout->addWidget(computeNormalsForHemiSphereButton);
    QObject::connect(computeNormalsForHemiSphereButton, SIGNAL(clicked(bool)), this, SLOT(NormalComputationForHemisphereRequested(bool)));

    //
    // Button for filtering the currently captured pointcloud and displaying it in the second window
    //
    QPushButton* filterPointCloudButton = new QPushButton("Filter Pointcloud");
    filterPointCloudButton->setMaximumWidth(200);
    layout->addWidget(filterPointCloudButton);
    QObject::connect(filterPointCloudButton, SIGNAL(clicked(bool)), this, SLOT(PointCloudFilterRequested(bool)));

    //
    // Button for saving the current input frame to disk
    //
    QPushButton* saveSnapshotButton = new QPushButton("Save Snapshot");
    saveSnapshotButton->setMaximumWidth(200);
    layout->addWidget(saveSnapshotButton);
    QObject::connect(saveSnapshotButton, SIGNAL(clicked(bool)), this, SLOT(SnapshotRequested(bool)));

    //
    // Button for loading a frame from disk
    //
    QPushButton* loadSnapshotButton = new QPushButton("Load Snapshot");
    loadSnapshotButton->setMaximumWidth(200);
    layout->addWidget(loadSnapshotButton);
    QObject::connect(loadSnapshotButton, SIGNAL(clicked(bool)), this, SLOT(LoadSnapshotRequested(bool)));


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

void MainWindow::FrameReady()
{
    DisplayDepthFrame();
    DisplayColorFrame();
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

        // TODO:
        // * save depth map
        // * save color image

        CopyFrameBuffer(&memory->gatherBuffer, &memory->snapshotBuffer);
        PointCloudHelpers::SaveSnapshot(&memory->snapshotBuffer);
        inspectionPointCloudDisplay->SetData(memory->snapshotBuffer.pointCloudBuffer, true /* with normals */);


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

    QPixmap pixmap = QPixmap::fromImage(QImage((uchar*)memory->gatherBuffer.depthBuffer,
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
