#include "MainWindow.h"
#include "ui_mainwindow.h"

#include "KinectGrabber.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    kinectGrabber = new KinectGrabber();

    kinectGrabber->ConnectToKinect();
    kinectGrabber->StartStream();

    QObject::connect(kinectGrabber, SIGNAL(ColorFrameAvailable(uchar*)), this, SLOT(DisplayColorFrame(uchar*)));
    QObject::connect(kinectGrabber, SIGNAL(DepthFrameAvailable(uchar*)), this, SLOT(DisplayDepthFrame(uchar*)));

}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::DisplayColorFrame(uchar *colorBuffer)
{
    int height = ui->colorDisplay->size().height();
    QPixmap pixmap = QPixmap::fromImage(QImage(colorBuffer,
                                               COLOR_WIDTH,
                                               COLOR_HEIGHT,
                                               QImage::Format_RGBA8888));
    ui->colorDisplay->setPixmap(pixmap.scaledToHeight(height));
}

void MainWindow::DisplayDepthFrame(uchar *depthBuffer)
{
    int height = ui->depthDisplay->size().height();

    QPixmap pixmap = QPixmap::fromImage(QImage(depthBuffer,
                                               DEPTH_WIDTH,
                                               DEPTH_HEIGHT,
                                               QImage::Format_Grayscale8));

    ui->depthDisplay->setPixmap(pixmap.scaledToHeight(height));
}
