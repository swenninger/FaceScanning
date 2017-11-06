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
    QObject::connect(kinectGrabber, SIGNAL(PointCloudDataAvailable(Vec3f*,RGB3f*,int)), this, SLOT(DisplayPointCloud(Vec3f*,RGB3f*,int)));


    colorDisplay = new QLabel();
    colorDisplay->setMinimumSize(300, 300);
    depthDisplay = new QLabel();
    depthDisplay->setMinimumSize(300, 300);
    count = 1;
    pointCloudDisplay = new PointCloudDisplay();

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
    drawColoredPointcloud->setChecked(true);

    ui->gridLayout->addWidget(depthDisplay, 0, 0, 1, 1);
    ui->gridLayout->addWidget(colorDisplay, 1, 0, 1, 1);
    ui->gridLayout->addWidget(pointCloudDisplay, 2, 0, 1, 1);
    ui->gridLayout->addWidget(settingsBox, 0, 1, 1, 3);
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

void MainWindow::DisplayPointCloud(Vec3f *p, RGB3f *c, int size)
{
    pointCloudDisplay->setData(p,c,size);
}
