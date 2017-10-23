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
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::DisplayColorFrame(uchar *colorBuffer)
{
    int height = ui->colorLabel->size().height();
    QPixmap pixmap = QPixmap::fromImage(QImage((uchar*) colorBuffer,
                                               COLOR_WIDTH,
                                               COLOR_HEIGHT,
                                               QImage::Format_RGBA8888));
    ui->colorLabel->setPixmap(pixmap.scaledToHeight(height));
}
