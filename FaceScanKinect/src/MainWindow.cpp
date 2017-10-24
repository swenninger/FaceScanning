#include "MainWindow.h"
#include "ui_mainwindow.h"

#include <QLabel>

#include "KinectGrabber.h"
#include "PointCloudDisplay.h"


MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    kinectGrabber = new KinectGrabber();

    QObject::connect(kinectGrabber, SIGNAL(ColorFrameAvailable(uchar*)), this, SLOT(DisplayColorFrame(uchar*)));
    QObject::connect(kinectGrabber, SIGNAL(DepthFrameAvailable(uchar*)), this, SLOT(DisplayDepthFrame(uchar*)));
    QObject::connect(kinectGrabber, SIGNAL(FPSStatusMessage(float)), this, SLOT(DisplayFPS(float)));

    colorDisplay = new QLabel();
    depthDisplay = new QLabel();
    pointCloudDisplay = new PointCloudDisplay();

    kinectGrabber->ConnectToKinect();
    kinectGrabber->StartStream();

    ui->gridLayout->addWidget(depthDisplay, 0, 0, 1, 1);
    ui->gridLayout->addWidget(colorDisplay, 1, 0, 1, 2);
    ui->gridLayout->addWidget(pointCloudDisplay, 0, 1, 1, 1);

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

void MainWindow::DisplayPointCloud(CameraSpacePoint *p, RGBQUAD *c, size_t size)
{
}
