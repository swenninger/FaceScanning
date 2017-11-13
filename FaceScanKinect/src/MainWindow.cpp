#include "MainWindow.h"
#include "ui_mainwindow.h"

#include <QLabel>
#include <QVBoxLayout>

#include "KinectGrabber.h"
#include "PointCloudDisplay.h"


MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
/*
    PointCloud c;
    LoadPointCloudFromFile("C:\\Users\\coruser\\Documents\\Stephan\\FaceScanning\\FaceScanKinect\\data\\points.txt", "C:\\Users\\coruser\\Documents\\Stephan\\FaceScanning\\FaceScanKinect\\data\\colors.txt", &c);
    pointCloudDisplay->setData(c.points, c.colors, c.size);
*/
    kinectGrabber = new KinectGrabber();

    QObject::connect(kinectGrabber, SIGNAL(ColorFrameAvailable(uchar*)), this, SLOT(DisplayColorFrame(uchar*)));
    QObject::connect(kinectGrabber, SIGNAL(DepthFrameAvailable(uchar*)), this, SLOT(DisplayDepthFrame(uchar*)));
    QObject::connect(kinectGrabber, SIGNAL(FPSStatusMessage(float)), this, SLOT(DisplayFPS(float)));
    QObject::connect(kinectGrabber, SIGNAL(PointCloudDataAvailable(Vec3f*,RGB3f*,size_t)), this, SLOT(DisplayPointCloud(Vec3f*,RGB3f*,size_t)));


    colorDisplay = new QLabel();
//    colorDisplay->setMinimumSize(200, 200);
    depthDisplay = new QLabel();
//    depthDisplay->setMinimumSize(200, 200);
    count = 1;
    pointCloudDisplay = new PointCloudDisplay();
    pointCloudDisplay->setMinimumSize(200, 200);

    pointCloudSaveRequested = false;
    pointCloudSaveDone = false;
    normalComputationRequested = false;

    inspectionPointCloudDisplay = new PointCloudDisplay();
    inspectionPointCloudDisplay->setMinimumSize(500, 500);
    inspectedPointCloud = {};

    kinectGrabber->ConnectToKinect();
    kinectGrabber->StartStream();

    QWidget* settingsBox = new QWidget(this);
    QVBoxLayout* layout  = new QVBoxLayout(settingsBox);
    layout->setSpacing(2);
    settingsBox->setLayout(layout);

    layout->addWidget(new QLabel("PointCloud Settings"));

    QCheckBox* drawNonTrackedPoints = new QCheckBox("Draw non-tracked points");
    layout->addWidget(drawNonTrackedPoints);
    QObject::connect(drawNonTrackedPoints, SIGNAL(stateChanged(int)), kinectGrabber, SLOT(RetrieveTrackedBodiesOnlySettingsChanged(int)));
    drawNonTrackedPoints->setChecked(false);

    QCheckBox* drawColoredPointcloud = new QCheckBox("Draw colored points");
    layout->addWidget(drawColoredPointcloud);
    QObject::connect(drawColoredPointcloud, SIGNAL(stateChanged(int)), pointCloudDisplay, SLOT(ColoredPointsSettingChanged(int)));
    QObject::connect(drawColoredPointcloud, SIGNAL(stateChanged(int)), inspectionPointCloudDisplay, SLOT(ColoredPointsSettingChanged(int)));
    drawColoredPointcloud->setChecked(true);

    QPushButton* saveButton = new QPushButton("Save Pointcloud to Disk");
    saveButton->setMaximumWidth(200);
    layout->addWidget(saveButton);
    QObject::connect(saveButton, SIGNAL(clicked(bool)), this, SLOT(PointCloudSaveRequested(bool)));

    QPushButton* loadButton = new QPushButton("Load Pointcloud from Disk");
    loadButton->setMaximumWidth(200);
    layout->addWidget(loadButton);
    QObject::connect(loadButton, SIGNAL(clicked(bool)), this, SLOT(PointCloudLoadRequested(bool)));

    QPushButton* computeNormalsButton = new QPushButton("Compute Normals");
    computeNormalsButton->setMaximumWidth(200);
    layout->addWidget(computeNormalsButton);
    QObject::connect(computeNormalsButton, SIGNAL(clicked(bool)), this, SLOT(NormalComputationRequested(bool)));



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

void MainWindow::DisplayColorFrame(uchar *colorBuffer)
{
    int height = colorDisplay->size().height();
    QPixmap pixmap = QPixmap::fromImage(QImage(colorBuffer,
                                               COLOR_WIDTH,
                                               COLOR_HEIGHT,
                                               QImage::Format_RGBA8888));
    colorDisplay->setPixmap(pixmap.scaledToHeight(height));
}

void MainWindow::DisplayDepthFrame(uchar *depthBuffer)
{
    int height = depthDisplay->size().height();

    QPixmap pixmap = QPixmap::fromImage(QImage(depthBuffer,
                                               DEPTH_WIDTH,
                                               DEPTH_HEIGHT,
                                               QImage::Format_Grayscale8));
    depthDisplay->setPixmap(pixmap.scaledToHeight(height));
}

void MainWindow::DisplayFPS(float fps)
{
    ui->statusBar->showMessage(QString::number(fps));
}

void MainWindow::DisplayPointCloud(Vec3f *p, RGB3f *c, size_t size)
{
    pointCloudDisplay->SetData(p,c,size);

    if (pointCloudSaveRequested) {

        pointCloudSaveRequested = false;
        pointCloudSaveDone = false;
        PointCloud pc = {};
        pc.size = size;
        pc.points = p;
        pc.colors = c;

        QString tmp1 = saveFilename;
        QString colorFile = tmp1.replace(".txt", "-colors.txt");
        QString pointFile = tmp1.replace("-colors.txt", "-points.txt");

        WritePointCloudToFile(pointFile.toStdString().c_str(), colorFile.toStdString().c_str(), pc);
        pointCloudSaveDone = true;
    }

    if (normalComputationRequested) {
        // TODO: copy data to inspectionpointclouddisplay
        // call compute normals
        inspectionPointCloudDisplay->ComputeNormals(p, c, size);
        normalComputationRequested = false;
    }
}

void MainWindow::PointCloudSaveRequested(bool)
{
    saveFilename = QFileDialog::getSaveFileName(this, "Select File to write pointcloud to", QDir(".").absolutePath(), "Text Files(*.txt)");
    pointCloudSaveRequested = true;
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

#include "util.h"

void MainWindow::NormalComputationRequested(bool)
{
    normalComputationRequested = true;
/*
    LoadPointCloud("..\\..\\..\\data\\pointclouds\\front-points.txt",
                   "..\\..\\..\\data\\pointclouds\\front-colors.txt",
                   &inspectedPointCloud);

*/
    inspectedPointCloud = GenerateSphere();


    DisplayPointCloud(inspectedPointCloud.points, inspectedPointCloud.colors, inspectedPointCloud.size);
}
