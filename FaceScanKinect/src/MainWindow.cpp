#include "MainWindow.h"
#include "ui_mainwindow.h"

#include <QLabel>
#include <QVBoxLayout>

#include "KinectGrabber.h"
#include "PointCloudDisplay.h"

#include "util.h"

#include "PointCloud.h"
#include "MemoryPool.h"

MainWindow::MainWindow(MemoryPool* memory, QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    this->memory = memory;

    ui->setupUi(this);
    kinectGrabber = new KinectGrabber(&memory->gatherBuffer);

    QObject::connect(kinectGrabber, SIGNAL(FPSStatusMessage(float)), this, SLOT(DisplayFPS(float)));

    QObject::connect(kinectGrabber, SIGNAL(FrameReady()), this, SLOT(FrameReady()));

    QObject::connect(this, SIGNAL(FileDestinationChosen()), this, SLOT(OnFileDestinationChosen()));

    colorDisplay = new QLabel();
    colorDisplay->setMinimumSize(200, 200);
    depthDisplay = new QLabel();
    depthDisplay->setMinimumSize(200, 200);
    pointCloudDisplay = new PointCloudDisplay();
    pointCloudDisplay->setMinimumSize(200, 200);

    pointCloudSaveRequested = false;
    pointCloudSaveDone = false;
    normalComputationRequested = false;
    pointCloudFilterRequested = false;
    snapshotRequested = false;

    inspectionPointCloudDisplay = new PointCloudDisplay();
    inspectionPointCloudDisplay->setMinimumSize(500, 500);
    inspectedPointCloud = {};
    pointCloudBuffer = {};

    kinectGrabber->ConnectToKinect();
    kinectGrabber->StartStream();

    QWidget* settingsBox = new QWidget(this);
    QVBoxLayout* layout  = new QVBoxLayout(settingsBox);
    layout->setSpacing(2);
    settingsBox->setLayout(layout);

    layout->addWidget(new QLabel("PointCloud Settings"));

    /*
     * Checkbox for toggling if we display/gather points that do not belong to tracked bodies
     */
    QCheckBox* drawNonTrackedPoints = new QCheckBox("Draw non-tracked points");
    layout->addWidget(drawNonTrackedPoints);
    QObject::connect(drawNonTrackedPoints, SIGNAL(stateChanged(int)), kinectGrabber, SLOT(RetrieveTrackedBodiesOnlySettingsChanged(int)));
    drawNonTrackedPoints->setChecked(false);

    /*
     * Checkbox for toggling if we display point colors
     */
    QCheckBox* drawColoredPointcloud = new QCheckBox("Draw colored points");
    layout->addWidget(drawColoredPointcloud);
    QObject::connect(drawColoredPointcloud, SIGNAL(stateChanged(int)), pointCloudDisplay, SLOT(ColoredPointsSettingChanged(int)));
    QObject::connect(drawColoredPointcloud, SIGNAL(stateChanged(int)), inspectionPointCloudDisplay, SLOT(ColoredPointsSettingChanged(int)));
    drawColoredPointcloud->setChecked(true);

    /*
     * Button for saving the currently captured Pointcloud to disk
     */
    QPushButton* saveButton = new QPushButton("Save Pointcloud to Disk");
    saveButton->setMaximumWidth(200);
    layout->addWidget(saveButton);
    QObject::connect(saveButton, SIGNAL(clicked(bool)), this, SLOT(PointCloudSaveRequested(bool)));

    /*
     * Button for loading a pointcloud from disk and displaying it in second window
     */
    QPushButton* loadButton = new QPushButton("Load Pointcloud from Disk");
    loadButton->setMaximumWidth(200);
    layout->addWidget(loadButton);
    QObject::connect(loadButton, SIGNAL(clicked(bool)), this, SLOT(PointCloudLoadRequested(bool)));

    /*
     * Button for computing the normals of the currently captured Pointcloud and displaying them in second window
     */
    QPushButton* computeNormalsButton = new QPushButton("Compute Normals");
    computeNormalsButton->setMaximumWidth(200);
    layout->addWidget(computeNormalsButton);
    QObject::connect(computeNormalsButton, SIGNAL(clicked(bool)), this, SLOT(NormalComputationRequested(bool)));

    /*
     * Button for generating a hemisphere and computing + displaying the normals for it
     * (for visually verifying the normal generation algorithm)
     */
    QPushButton* computeNormalsForHemiSphereButton = new QPushButton("Compute Normals for Hemisphere");
    computeNormalsForHemiSphereButton->setMaximumWidth(200);
    layout->addWidget(computeNormalsForHemiSphereButton);
    QObject::connect(computeNormalsForHemiSphereButton, SIGNAL(clicked(bool)), this, SLOT(NormalComputationForHemisphereRequested(bool)));

    /*
     * Button for filtering the currently captured pointcloud and displaying it in the second window
     */
    QPushButton* filterPointCloudButton = new QPushButton("Filter Pointcloud");
    filterPointCloudButton->setMaximumWidth(200);
    layout->addWidget(filterPointCloudButton);
    QObject::connect(filterPointCloudButton, SIGNAL(clicked(bool)), this, SLOT(PointCloudFilterRequested(bool)));



    QPushButton* saveSnapshotButton = new QPushButton("Save Snapshot");
    saveSnapshotButton->setMaximumWidth(200);
    layout->addWidget(saveSnapshotButton);
    QObject::connect(saveSnapshotButton, SIGNAL(clicked(bool)), this, SLOT(SnapshotRequested(bool)));


    QPushButton* loadSnapshotButton = new QPushButton("Load Snapshot");
    loadSnapshotButton->setMaximumWidth(200);
    layout->addWidget(loadSnapshotButton);
    QObject::connect(loadSnapshotButton, SIGNAL(clicked(bool)), this, SLOT(LoadSnapshotRequested(bool)));


    /*
     * Inputs for controlling the pointcloud filter algorithm
     */
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

    // Set widget positions in the grid
    ui->gridLayout->addWidget(depthDisplay,                0, 0, 1, 1);
    ui->gridLayout->addWidget(colorDisplay,                1, 0, 1, 1);
    ui->gridLayout->addWidget(pointCloudDisplay,           2, 0, 1, 1);
    ui->gridLayout->addWidget(settingsBox,                 0, 1, 1, 2);
    ui->gridLayout->addWidget(inspectionPointCloudDisplay, 2, 1, 1, 1);
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
        CopyPointCloudBuffer(&memory->gatherBuffer.pointcloudBuffer, &memory->inspectionBuffer);
        PointCloudHelpers::CreateAndStartNormalWorker(&memory->inspectionBuffer, this);
        normalComputationRequested = false;
    }

    if (pointCloudFilterRequested) {
        CopyPointCloudBuffer(&memory->gatherBuffer.pointcloudBuffer, &memory->inspectionBuffer);
        PointCloudHelpers::CreateAndStartFilterWorker(&memory->inspectionBuffer, &memory->filterBuffer, this);
        pointCloudFilterRequested = false;
    }

    if (snapshotRequested) {

        // TODO:
        // * filter
        // * normals
        // * save pointcloud file
        // * save depth map
        // * save color image

        CopyFrameBuffer(&memory->gatherBuffer, &memory->snapshotBuffer);
        PointCloudHelpers::SaveSnapshot(&memory->snapshotBuffer);

//        CopyPointCloud(frame.pointCloud, &inspectedPointCloud);
//        inspectionPointCloudDisplay->TakeSnapshot(inspectedPointCloud);

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
    pointCloudDisplay->SetData(&memory->gatherBuffer.pointcloudBuffer);
}

void MainWindow::DisplayFPS(float fps)
{
    ui->statusBar->showMessage(QString::number(fps));
}

void MainWindow::PointCloudSaveRequested(bool)
{
    pointCloudSaveRequested = true;
    saveFilename = QFileDialog::getSaveFileName(this, "Select File to write pointcloud to", QDir(".").absolutePath(), "Text Files(*.txt)");

    emit FileDestinationChosen();
}

void MainWindow::OnFileDestinationChosen()
{
    QString tmp1 = saveFilename;
    QString colorFile = tmp1.replace(".txt", "-colors.txt");
    QString pointFile = tmp1.replace("-colors.txt", "-points.txt");

    WritePointCloudToFile(pointFile.toStdString().c_str(), colorFile.toStdString().c_str(), pointCloudBuffer);
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

    // TODO: check normal loading

    Vec3f* normals = nullptr;

//    LoadSnapshot(loadFileName.toStdString().c_str(), &inspectedPointCloud, &normals);
    PointCloudHelpers::LoadSnapshot(loadFileName.toStdString().c_str(), &memory->snapshotBuffer.pointcloudBuffer);
    inspectionPointCloudDisplay->SetData(&memory->snapshotBuffer.pointcloudBuffer, true /* with normals */);

//    inspectionPointCloudDisplay->SetData(inspectedPointCloud.points, inspectedPointCloud.colors, normals, inspectedPointCloud.size);
//    inspectionPointCloudDisplay->SetData(inspectedPointCloud.points, inspectedPointCloud.colors, inspectedPointCloud.size);
}

void MainWindow::PointCloudLoadRequested(bool)
{
    QString loadFileName = QFileDialog::getOpenFileName(this, "Select Point File to read", "..\\..\\data\\pointclouds", "Point Files(*-points.txt)", nullptr, QFileDialog::DontUseNativeDialog);

    if (loadFileName.isNull() || loadFileName.isEmpty()) {
        return;
    }

    std::string pointFile = loadFileName.toStdString();
    std::string colorFile = loadFileName.replace("-points.txt", "-colors.txt").toStdString();

    LoadPointCloud(pointFile, colorFile, &inspectedPointCloud);

    inspectionPointCloudDisplay->SetData(inspectedPointCloud.points, inspectedPointCloud.colors, inspectedPointCloud.size);
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
